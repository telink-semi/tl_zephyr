/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <timer.h>

/***************************************
 * Timer 0 - high priority ISR
 * Timer 1 - low priority ISR
 ***************************************/

#define TIMER_CAPTURE_TICKS 40000000
#define TIMER0_ISR_NUMBER   4
#define TIMER1_ISR_NUMBER   3

static size_t counter;

static void high_prio_timer_start(void)
{
	timer_set_init_tick(TIMER0, 0);
	timer_start(TIMER0);
}

static void high_prio_timer_stop(void)
{
	timer_stop(TIMER0);
}

static void high_prio_timer_isr(volatile size_t *cnt)
{
	timer_clr_irq_status(FLD_TMR0_MODE_IRQ);
	printk("Timer 0 ISR - High priority, counter = %zu\n", *cnt);
	*cnt += 1;
}

static void low_prio_timer_isr(volatile size_t *cnt)
{
	timer_clr_irq_status(FLD_TMR1_MODE_IRQ);
	printk("Timer 1 ISR - Low priority start\n");
	*cnt = 0;
	high_prio_timer_start();
	for (; *cnt < 3;) {
	}
	high_prio_timer_stop();
	printk("Timer 1 ISR - Low priority finish\n");
}

static void high_prio_timer_init(void)
{
	timer_set_cap_tick(TIMER0, TIMER_CAPTURE_TICKS / 10);
	timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
	timer_set_irq_mask(FLD_TMR0_MODE_IRQ);
	IRQ_CONNECT(CONFIG_2ND_LVL_ISR_TBL_OFFSET + TIMER0_ISR_NUMBER, 2, high_prio_timer_isr,
		    &counter, 0);
	riscv_plic_set_priority(IRQ_TO_L2(TIMER0_ISR_NUMBER), 2);
	riscv_plic_irq_enable(IRQ_TO_L2(TIMER0_ISR_NUMBER));
}

static void low_prio_timer_init(void)
{
	timer_set_cap_tick(TIMER1, TIMER_CAPTURE_TICKS);
	timer_set_mode(TIMER1, TIMER_MODE_SYSCLK);
	timer_set_irq_mask(FLD_TMR1_MODE_IRQ);
	IRQ_CONNECT(CONFIG_2ND_LVL_ISR_TBL_OFFSET + TIMER1_ISR_NUMBER, 1, low_prio_timer_isr,
		    &counter, 0);
	riscv_plic_set_priority(IRQ_TO_L2(TIMER1_ISR_NUMBER), 1);
	riscv_plic_irq_enable(IRQ_TO_L2(TIMER1_ISR_NUMBER));
	/* start it now */
	timer_set_init_tick(TIMER1, 0);
	timer_start(TIMER1);
}

static void show_isr_configuration(void)
{
	uint32_t mmisc_ctl = csr_read(0x7d0);

	printk("mmisc_ctl %08x [vector mode %s]\n", mmisc_ctl,
	       mmisc_ctl & 0x2 ? "enabled" : "disabled");

	const uint32_t *const plic_base = (uint32_t *)DT_REG_ADDR(DT_NODELABEL(plic0));
	const size_t plic_isr_num = DT_PROP(DT_NODELABEL(plic0), riscv_ndev);

	printk("PLIC base address %p\n", plic_base);
	printk("PLIC_FEN %08x [vector mode %s, preemptive priority interrupt %s]\n", *plic_base,
	       *plic_base & 0x2 ? "enabled" : "disabled",
	       *plic_base & 0x1 ? "enabled" : "disabled");
	printk("PLIC_PRI:\n");
	for (size_t i = 1; i <= plic_isr_num; i++) {
		if (*(plic_base + i)) {
			printk("[%02zu] %u\n", i, *(plic_base + i));
		}
	}
	printk("PLIC_IE:");
	for (size_t i = 1; i <= plic_isr_num; i++) {
		size_t word = i / 32;
		uint8_t bit = i % 32;

		if (*((uint32_t *)((uint8_t *)plic_base + 0x2000) + word) & (1 << bit)) {
			printk(" %02zu", i);
		}
	}
	printk("\n");
}

int main(void)
{
	printk("app started\n");
	high_prio_timer_init();
	low_prio_timer_init();
	show_isr_configuration();
	return 0;
}
