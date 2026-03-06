/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bootutil/mcuboot_status.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/irq.h>


#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "bootutil/mcuboot_status.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <ext_driver/ext_pm.h>


#define PLIC_PRIO (0xe4000000)
#define PLIC_IRQS (CONFIG_NUM_IRQS - CONFIG_2ND_LVL_ISR_TBL_OFFSET)
#define BOOT_FLAG_ADR   0x1f8000
static void restore_all_irq_priorities(void)
{
    volatile uint32_t *prio = (volatile uint32_t *)PLIC_PRIO;
    int i;
    for( i=1; i<PLIC_IRQS; i++)
    {
        *prio = 1U;
        prio++;
    }
}

#define USER_PARTITION		user_para_partition
#define USER_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(USER_PARTITION)
#define USER_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(USER_PARTITION)
#define USER_PARTITION_SIZE 	FIXED_PARTITION_SIZE(USER_PARTITION)

#define SLOT0_PARTITION		slot0_partition
#define SLOT0_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(SLOT0_PARTITION)
#define SLOT0_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(SLOT0_PARTITION)
#define SLOT0_ZB_OFFSET     0x140000

const struct device * flash_para_dev = USER_PARTITION_DEVICE;
const struct device * flash_slot0_dev = SLOT0_PARTITION_DEVICE;
const uint8_t zb_fw_flag[4]={ 0x4b, 0x4e, 0x4c, 0x54};
uint8_t zb_slot0_flag[4];


#define USER_MATTER_PAIR_VAL    0x55  // jump to matter

#define USER_INIT_VAL           0xff  // init state or others will go into zb
#define USER_ZB_SW_VAL          0xaa  // jump to matter,use XIP
#define USER_MATTER_BACK_ZB     0xa0  // only commisiion fail will back to zb

#define ZB_FW_FLAG_OFFSET       0x20 //telink fw valid flag offset .

#define FLASH_ADR_BASE_OFFSET	0x20000000 // telink flash base adr

void * dual_mode_start_proc(void * boot_adr)
{
	void *start;
    /* Read the boot flag from the user partition to determine the boot behavior */
    uint8_t boot_flag = 0;
    flash_read(flash_para_dev, USER_PARTITION_OFFSET, &boot_flag, 1);
    printk("boot flag is  %x \n",boot_flag);

    /* Get the Zigbee firmware flag from slot1 partition */
    flash_read(flash_slot0_dev, SLOT0_PARTITION_OFFSET + SLOT0_ZB_OFFSET + ZB_FW_FLAG_OFFSET, zb_slot0_flag, sizeof(zb_slot0_flag));
    if (memcmp(zb_slot0_flag, zb_fw_flag, sizeof(zb_fw_flag))){
        /* Zigbee firmware flag not found, boot to Matter */
        printk("Zigbee flag not found \n");
        start = boot_adr;
    }else{
        if( boot_flag == USER_MATTER_PAIR_VAL ){
            /* only paired will switch to matter , Commissioning success flag  */
            start = boot_adr;
        }else{
            /*others it will go into zigbee */
            restore_all_irq_priorities();
            irq_lock();
            reg_irq_src0=0;
            reg_irq_src1=0;
            core_interrupt_disable();
            start = (void *)(FLASH_ADR_BASE_OFFSET + SLOT0_PARTITION_OFFSET + SLOT0_ZB_OFFSET);
        }
    }

    // Print the start address for debugging
    printk("start adr is %x \n",start);
	return start;
}

#define BOOTLOADER_MCUBOOT_ROM_START_OFFSET             0x200

void mcuboot_status_change(mcuboot_status_type_t status)
{
	if (status == MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND) {
		uintptr_t app_start_addr = DT_FIXED_PARTITION_ADDR(DT_NODELABEL(slot0_partition)) +
			BOOTLOADER_MCUBOOT_ROM_START_OFFSET;
		void *boot_app = (void *)app_start_addr;
        boot_app = dual_mode_start_proc((void *)app_start_addr);
		irq_lock();
		((void (*)(void))boot_app)();
	}
}
