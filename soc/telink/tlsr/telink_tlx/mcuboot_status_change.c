/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bootutil/mcuboot_status.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/fixed-partitions.h>
#if CONFIG_WATCHDOG_AUTO
#include <zephyr/drivers/watchdog.h>
#endif /* CONFIG_WATCHDOG_AUTO */
#include <zephyr/irq.h>

#if CONFIG_DUAL_MODE == CONFIG_ACTION_DUAL_MODE || CONFIG_DUAL_MODE == CONFIG_AUTO_SWITCH_DUAL_MODE
#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "bootutil/mcuboot_status.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <ext_driver/ext_pm.h>

#define ZIGBEE_PARTITION	slot0_zb_partition
#define ZIGBEE_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(ZIGBEE_PARTITION)
#define ZIGBEE_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(ZIGBEE_PARTITION)
#define ZIGBEE_PARTITION_SIZE 	FIXED_PARTITION_SIZE(ZIGBEE_PARTITION) 
// fw magic number
#define ZB_FW_FLAG_OFFSET       0x20

#define DUAL_MODE_PARTITION		dual_mode_partition
#define DUAL_MODE_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(DUAL_MODE_PARTITION)
#define DUAL_MODE_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(DUAL_MODE_PARTITION)
#define DUAL_MODE_PARTITION_SIZE 	FIXED_PARTITION_SIZE(DUAL_MODE_PARTITION) 

// init mode will jump to matter, if zigbee trigger action will jump to zigbee
#define MODE_VAL_INIT           0xff
#define ACTION_SWITCH_INIT      0xff
// after matter paired , it will go to matter, only if trigger action.
#define MODE_VAL_MATTER_PAIR    0x55
#define ACTION_SWITCH_ZIGBEE    0xaa

// after zb paired , it will go to zb, only if trigger action.
#define MODE_VAL_ZB_PAIR        0xaa
#define ACTION_SWITCH_MATTER    0x55

const struct device * flash_para_dev = DUAL_MODE_PARTITION_DEVICE;
const struct device * flash_zb_dev = ZIGBEE_PARTITION_DEVICE;
const uint8_t zb_magic_flag[4]={ 0x4b, 0x4e, 0x4c, 0x54};

static void restore_all_irq_priorities(void)
{

#define PLIC_PRIO (0xc4000000)
#define PLIC_IRQS (CONFIG_NUM_IRQS - CONFIG_2ND_LVL_ISR_TBL_OFFSET)

    volatile uint32_t *prio = (volatile uint32_t *)PLIC_PRIO;
    int i;
    for(i=1;i<PLIC_IRQS;i++)
    {
        *prio = 1U;
        prio++;
    }
}

static void jump_zb_prepare(void)
{
    restore_all_irq_priorities();
    irq_lock();
    reg_irq_src0=0;
    reg_irq_src1=0;
    core_interrupt_disable();
}
#endif

#if CONFIG_DUAL_MODE == CONFIG_ACTION_DUAL_MODE
static uint8_t jump_zb_dispatch(uint8_t *flag)
{
    uint8_t mode = flag[0];
    uint8_t action = flag[1];
    if(mode == MODE_VAL_INIT){
        // init mode is matter(may changed) , so should return 0, only action trigger
        if(action == ACTION_SWITCH_INIT){
            return 0;
        }else if(action == ACTION_SWITCH_ZIGBEE){
            return 1;
        }else if(action == ACTION_SWITCH_MATTER){
            return 0;
        }else{
            return 0;
        }
    }else if(mode == MODE_VAL_ZB_PAIR && action != ACTION_SWITCH_MATTER){
        // only zigbee paired , and not trigger to matter.
        return 1;
    }else if (mode == MODE_VAL_MATTER_PAIR && action == ACTION_SWITCH_ZIGBEE){
        return 1;
    }else{
        // other wise , it will jump to matter
        return 0;
    }
}

#elif CONFIG_DUAL_MODE == CONFIG_AUTO_SWITCH_DUAL_MODE

static uint8_t jump_zb_dispatch(uint8_t *flag)
{
    uint8_t mode = flag[0];
    if(mode != MODE_VAL_MATTER_PAIR){
        /* only matter paired , will jump to matter , otherwise it will jump to zb */ 
        return 1;
    }else {
        /* if mode is matter paried , will jump to matter */
        return 0;
    }
}
#endif

#define BOOTLOADER_MCUBOOT_ROM_START_OFFSET             0x200

void mcuboot_status_change(mcuboot_status_type_t status)
{
	if (status == MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND) {
#if CONFIG_DUAL_MODE == CONFIG_ACTION_DUAL_MODE || CONFIG_DUAL_MODE == CONFIG_AUTO_SWITCH_DUAL_MODE
        uintptr_t app_start_addr ;
    	/* Get the Zigbee firmware flag from slot1 partition */
    	uint8_t zb_fw_flag[4];
    	flash_read(flash_zb_dev, ZIGBEE_PARTITION_OFFSET + ZB_FW_FLAG_OFFSET, zb_fw_flag, sizeof(zb_fw_flag));
    	if (memcmp(zb_fw_flag, zb_magic_flag, sizeof(zb_magic_flag))){
        	/* Zigbee firmware flag not found, boot to Matter */
        	//printk("Zigbee flag not found, jump to matter \n");
        	// jump to matter,app_start_addr is init is matter.
			app_start_addr = DT_FIXED_PARTITION_ADDR(DT_NODELABEL(slot0_partition)) +
			BOOTLOADER_MCUBOOT_ROM_START_OFFSET;
    	}else{
        	/* Read the boot flag from the user partition to determine the boot behavior */
        	uint8_t boot_flag[2]={0xff,0xff};
        	flash_read(flash_para_dev, DUAL_MODE_PARTITION_OFFSET, boot_flag, 2);
        	printk("boot flag is  %x , %x \n",boot_flag[0],boot_flag[1]);

        	if( jump_zb_dispatch(boot_flag) ){
				/* Switch to Zigbee */
				jump_zb_prepare();
				app_start_addr = DT_FIXED_PARTITION_ADDR(DT_NODELABEL(slot0_zb_partition));
			}else {
				/* Switch to Matter */
				app_start_addr = DT_FIXED_PARTITION_ADDR(DT_NODELABEL(slot0_partition)) +
				BOOTLOADER_MCUBOOT_ROM_START_OFFSET;
			}
    	}

		// Print the start address for debugging
		printk("start adr is %x \n",app_start_addr);
#else
		uintptr_t app_start_addr = DT_FIXED_PARTITION_ADDR(DT_NODELABEL(slot0_partition)) +
			BOOTLOADER_MCUBOOT_ROM_START_OFFSET;
        
#endif
		void *boot_app = (void *)app_start_addr;

		irq_lock();
        clock_set_all_clock_to_default();
#if CONFIG_WATCHDOG_AUTO
		const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

		wdt_disable(wdt);
#endif /* CONFIG_WATCHDOG_AUTO */
		((void (*)(void))boot_app)();
	}
}
