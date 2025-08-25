/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <timer.h>

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;

/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios,
						     {0});
                             
static struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios,
						     {1});

static struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios,
						     {2});

/* Button interrupt handler */
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button ISR Start\n");
    gpio_pin_toggle_dt(&led1);
	delay_ms(50);
    gpio_pin_toggle_dt(&led1);
	printk("Button ISR Finish\n");
}

/* Timer0 interrupt handler */
static void timer0_isr(const struct gpio_dt_spec *led)
{
    if (timer_get_irq_status(FLD_TMR0_MODE_IRQ)){
		printk("Timer 0 ISR Start\n");
		gpio_pin_toggle_dt(led);
		timer_clr_irq_status(FLD_TMR0_MODE_IRQ); //Clear IRQ status
		delay_ms(150);
        gpio_pin_toggle_dt(led);
		printk("Timer 0 ISR Finish\n");
	}
}

/* Timer1 interrupt handler */
static void timer1_isr(const struct gpio_dt_spec *led2)
{
    if (timer_get_irq_status(FLD_TMR1_MODE_IRQ)){
		printk("Timer 1 ISR Start\n");
		gpio_pin_toggle_dt(led2);
		timer_clr_irq_status(FLD_TMR1_MODE_IRQ); //Clear IRQ status
		delay_ms(80);
        gpio_pin_toggle_dt(led2);
		printk("Timer 1 ISR Finish\n");
	}
}

/* Print current PLIC and interrupt configuration */
static void show_isr_configuration(void)
{
	printk("mmisc_ctl %08x [vector mode %s]\n", (uint32_t) csr_read(mmisc_ctl),
		(uint32_t) csr_read(mmisc_ctl) & 0x2 ? "enabled" : "disabled");

	const uint32_t * const plic_base = (uint32_t *)DT_REG_ADDR(DT_NODELABEL(plic0));

	printk("PLIC base address %p\n", plic_base);

	uint32_t plic_fen = *(plic_base);

	printk("PLIC_FEN %08x [vector mode %s, preemptive priority interrupt %s]\n",
		plic_fen,
		plic_fen & 0x2 ? "enabled" : "disabled",
		plic_fen & 0x1 ? "enabled" : "disabled");


	printk("PLIC_PRI:\n");
	for (size_t i = 1; i <= DT_PROP(DT_NODELABEL(plic0), riscv_ndev); i++) {
		if (*(plic_base + i)) {
			printk("[%02zu] %u\n", i, *(plic_base + i));
		}
	}

	printk("PLIC_IE:");
	for (size_t i = 1; i <= DT_PROP(DT_NODELABEL(plic0), riscv_ndev); i++) {
		size_t word = i / 32;
		uint8_t bit = i % 32;

		if (*((uint32_t *)((uint8_t *)plic_base + 0x2000) + word) & (1 << bit)) {
			printk(" %02zu",i);
		}
	}
	printk("\n");
}

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	riscv_plic_set_priority(IRQ_GPIO_IRQ3, 3);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	if (led.port && !device_is_ready(led.port)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, led.port->name);
		led.port = NULL;
	}
	if (led.port) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led.port->name, led.pin);
			led.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led.port->name, led.pin);
		}
	}

    /* Initialize LED1 */
    if (led1.port && !gpio_is_ready_dt(&led1)) {
        printk("Error: LED1 device %s is not ready; ignoring it\n",
               led1.port->name);
        led1.port = NULL;
    }
    if (led1.port) {
        ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT);
        if (ret != 0) {
            printk("Error %d: failed to configure LED1 device %s pin %d\n",
                   ret, led1.port->name, led1.pin);
            led1.port = NULL;
        } else {
            printk("Set up LED1 at %s pin %d\n", led1.port->name, led1.pin);
        }
    }

	/* Initialize LED2 */
    if (led2.port && !gpio_is_ready_dt(&led2)) {
        printk("Error: LED2 device %s is not ready; ignoring it\n",
               led2.port->name);
        led2.port = NULL;
    }
    if (led2.port) {
        ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT);
        if (ret != 0) {
            printk("Error %d: failed to configure LED2 device %s pin %d\n",
                   ret, led2.port->name, led2.pin);
            led2.port = NULL;
        } else {
            printk("Set up LED2 at %s pin %d\n", led2.port->name, led2.pin);
        }
    }

	/* Initialize and configure timers */
	if (led.port) {
		/* Timer0 configuration */
		timer_set_init_tick(TIMER0, 0);
		timer_set_cap_tick(TIMER0, 1000*sys_clk.pclk * 300);	//300ms
		timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
		timer_set_irq_mask(FLD_TMR0_MODE_IRQ);
		IRQ_CONNECT(CONFIG_2ND_LVL_ISR_TBL_OFFSET + IRQ_TIMER0, 2, timer0_isr, &led, 0);
		riscv_plic_set_priority(IRQ_TIMER0, 1);
		riscv_plic_irq_enable(IRQ_TIMER0);

		/* Timer1 configuration */
		timer_set_init_tick(TIMER1, 0);
		timer_set_cap_tick(TIMER1, 1000*sys_clk.pclk * 150);	//150ms
		timer_set_mode(TIMER1, TIMER_MODE_SYSCLK);
		timer_set_irq_mask(FLD_TMR1_MODE_IRQ);
		IRQ_CONNECT(CONFIG_2ND_LVL_ISR_TBL_OFFSET + IRQ_TIMER1, 2, timer1_isr, &led2, 0);
		riscv_plic_set_priority(IRQ_TIMER1, 2);
		riscv_plic_irq_enable(IRQ_TIMER1);

		/* Start timers */
		timer_start(TIMER0);
		timer_start(TIMER1);
	}

	show_isr_configuration();

	return 0;
}
