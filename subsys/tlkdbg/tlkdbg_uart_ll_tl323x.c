/*
 * Copyright (c) 2026 Telink Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 *
 * TL323x low-level UART setup for tlkdbg.
 *
 * The component owns the UART (the Zephyr console/log subsystem is disabled
 * when CONFIG_TLKDBG is on). Pins are muxed through Zephyr pinctrl from the
 * "telink,tlkdbg-uart" devicetree node; baud/DMA come from the Telink HAL.
 *
 * No interrupt resource is used: the UART TX interrupt mask stays 0 and the
 * DMA channel is driven in normal (polled) mode only.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

#include "tlkdbg_cfg.h"

#include "uart.h"
#include "dma.h"
#include "lib/include/clock.h"

#define TLKDBG_UART_PORT    ((uart_num_e)CONFIG_TLKDBG_UART_INSTANCE)
#define TLKDBG_UART_DMA_CHN ((dma_chn_e)CONFIG_TLKDBG_UART_DMA_CH)

#if DT_HAS_COMPAT_STATUS_OKAY(telink_tlkdbg_uart)
#define TLKDBG_UART_DT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(telink_tlkdbg_uart)
PINCTRL_DT_DEFINE(TLKDBG_UART_DT_NODE);
#endif

void tlkdbg_uart_hw_init(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(telink_tlkdbg_uart)
	(void)pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(TLKDBG_UART_DT_NODE),
				  PINCTRL_STATE_DEFAULT);
#endif

	uart_clk_en(TLKDBG_UART_PORT);
	uart_reset(TLKDBG_UART_PORT);
	dma_chn_dis(TLKDBG_UART_DMA_CHN);

	unsigned short div;
	unsigned char bwpc;

	uart_cal_div_and_bwpc(TLKDBG_UART_BAUDRATE,
			      (unsigned int)sys_clk.pclk * 1000u * 1000u, &div, &bwpc);
	uart_init(TLKDBG_UART_PORT, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);
	uart_set_tx_dma_config(TLKDBG_UART_PORT, TLKDBG_UART_DMA_CHN);

	uart_clr_irq_status(TLKDBG_UART_PORT, UART_TXDONE_IRQ_STATUS);

	/* Polled TX only: this clears every interrupt source on the UART. */
	uart_set_irq_mask(TLKDBG_UART_PORT, (uart_irq_mask_e)0);
}
