/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <bootutil/bootutil_log.h>
#include <zephyr/sys/crc.h>

#if CONFIG_SOC_RISCV_TELINK_TL323X
#include "efuse.h"
#include "gpio.h"
#include <zephyr/dt-bindings/pinctrl/tl323x-pinctrl.h>
#endif

BOOT_LOG_MODULE_REGISTER(telink_tlx_mcuboot);

static bool telink_tlx_mcu_boot_startup(void);

void __wrap_main(void)
{
	if (telink_tlx_mcu_boot_startup()) {
		extern void __real_main(void);

		__real_main();
	}
}

/* Vendor specific code during MCUBoot startup */
static bool telink_tlx_mcu_boot_startup(void)
{
	bool result = true; /* run MCUBoot main */

#if CONFIG_SOC_RISCV_TELINK_TL323X
	bool show_chip_id = false;

	/* Check if Console UART RX line is at low level - shorted to ground */
	const struct device *const uart_con = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (device_is_ready(uart_con)) {

	/* Use the uart corresponding to your zephyr_console configuration */
		#define UART_RX_PINMUX \
			DT_PROP(DT_PINCTRL_BY_IDX(DT_NODELABEL(uart0), 0, 1), pinmux)
		gpio_pin_e uart_rx = TLX_PINMUX_GET_PIN(UART_RX_PINMUX);

		/* Disable The UART RX PIN */
		gpio_set_low_level(uart_rx);
		gpio_output_dis(uart_rx);
		gpio_function_dis(uart_rx);

		/* Enable The UART RX PIN GPIO Function */
		gpio_function_en(uart_rx);
		gpio_input_en(uart_rx);
		gpio_set_up_down_res(uart_rx, GPIO_PIN_PULLUP_10K);

		/* Check if Console UART RX PIN is at low level - shorted to ground */
		if (gpio_get_level(uart_rx) == 0) {
			show_chip_id = true;
		}
	} else {
		BOOT_LOG_ERR("uart console not ready");
	}

	if (show_chip_id) {
		extern drv_api_status_e efuse_get_ieee_addr(unsigned char *chip_id_buff);
		uint8_t chip_id[21] = {0};
		uint8_t ieee_addr[8] = {0};
		if (efuse_get_ieee_addr(ieee_addr) == DRV_API_SUCCESS) {

			memcpy(chip_id + 2, ieee_addr, 8);
			uint16_t chip_id_crc = crc16_itu_t(0, chip_id + 2, 16);

			chip_id[0] = 0xaa;
			chip_id[1] = 0x12;
			chip_id[18] = chip_id_crc & 0x00ff;
			chip_id[19] = chip_id_crc >> 8;
			chip_id[20] = 0x55;
			print_buffer(chip_id, sizeof(chip_id));
		} else {
			BOOT_LOG_ERR("chip id read error");
		}
	}
#endif /* CONFIG_SOC_RISCV_TELINK_TL323X */
	return result;
}

void print_buffer(uint8_t *buffer, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		printk("%c", buffer[i]);
	}
}
