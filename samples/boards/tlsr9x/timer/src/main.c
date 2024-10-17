/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/pm/device.h>
#include <timer.h>
#include <zephyr/irq.h>
#include <string.h>
#include <stdint.h>
#if CONFIG_SOC_RISCV_TELINK_B91
#include "driver_b91.h"
#elif CONFIG_SOC_RISCV_TELINK_B92
#include "driver_b92.h"
#else
#error "App is not suitable for chosen SoC!"
#endif

/* GPIO driver configuration structure */
struct gpio_b9x_config {
	struct gpio_driver_config common;
	uint32_t gpio_base;
};

/* The devicetree node identifier for the "timer_settings" alias. */
#define TIMER_CONFIGURATION_NODE DT_PATH(timer_settings)

#define TIMER_GPIO_TRIGGER_MODE  DT_PROP(TIMER_CONFIGURATION_NODE, timer_trigger_mode)
#define TIMER_GPIO_WIDTH_MODE    DT_PROP(TIMER_CONFIGURATION_NODE, timer_width_mode)
#define TIMER_TICK_MODE          DT_PROP(TIMER_CONFIGURATION_NODE, timer_tick_mode)
/* Select the timer mode */
#define TIMER_MODE               TIMER_GPIO_WIDTH_MODE

#define TIMER_GPIO_TRIGGER_TICK  DT_PROP(TIMER_CONFIGURATION_NODE, timer_gpio_trigger_tick)

#define TIMER0_IRQ_NUM           (IRQ4_TIMER0 + CONFIG_2ND_LVL_ISR_TBL_OFFSET)
#define TIMER1_IRQ_NUM           (IRQ3_TIMER1 + CONFIG_2ND_LVL_ISR_TBL_OFFSET)

/* Get GPIO instance */
#define GET_GPIO_BASE(dev) (((const struct gpio_b9x_config *)dev->config)->gpio_base)

/* Get GPIO port number: port A - 0, port B - 1, ..., port F - 5  */
#define GET_PORT_NUM(gpio) ((uint8_t)(((uint32_t)gpio - DT_REG_ADDR(DT_NODELABEL(gpioa))) \
						/ DT_REG_SIZE(DT_NODELABEL(gpioa))))

/* Get GPIO IRQ number defined in dts */
#define GPIO_PORT_ADDR_STEP 0x100
#define GET_TIMER_CAPT_PIN(gpio_dt) ((uint32_t)(GET_PORT_NUM(GET_GPIO_BASE(gpio_dt.port)) \
						* GPIO_PORT_ADDR_STEP) | BIT(gpio_dt.pin))

/* The devicetree node identifier for the "leds" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

#define TIM0_TRIG_NODE DT_ALIAS(tim0_trig_out)
#define TIM1_TRIG_NODE DT_ALIAS(tim1_trig_out)
#define TIM0_CAPT_NODE DT_ALIAS(tim0_capt_in)
#define TIM1_CAPT_NODE DT_ALIAS(tim1_capt_in)

/* Get Timers "gpios" instances */
static const struct gpio_dt_spec tim0_trig = GPIO_DT_SPEC_GET(TIM0_TRIG_NODE, gpios);
static const struct gpio_dt_spec tim1_trig = GPIO_DT_SPEC_GET(TIM1_TRIG_NODE, gpios);
static const struct gpio_dt_spec tim0_capt = GPIO_DT_SPEC_GET(TIM0_CAPT_NODE, gpios);
static const struct gpio_dt_spec tim1_capt = GPIO_DT_SPEC_GET(TIM1_CAPT_NODE, gpios);

/* Get Leds "gpios" instances */
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led4 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

#if (TIMER_MODE == TIMER_GPIO_TRIGGER_MODE)
static int timer0_irq_cnt;
static int timer1_irq_cnt;
#elif (TIMER_MODE == TIMER_GPIO_WIDTH_MODE)
static unsigned int timer0_gpio_width;
static unsigned int timer1_gpio_width;
#endif

_attribute_ram_code_sec_ void timer0_irq_handler(void)
{
	if (timer_get_irq_status(TMR_STA_TMR0)) {
		timer_clr_irq_status(TMR_STA_TMR0);
#if (TIMER_MODE == TIMER_GPIO_TRIGGER_MODE)
		timer0_irq_cnt++;
#elif (TIMER_MODE == TIMER_GPIO_WIDTH_MODE)
		timer0_gpio_width = timer0_get_gpio_width();
		timer0_set_tick(0);
#endif
		gpio_pin_toggle_dt(&tim0_trig);
		gpio_pin_toggle_dt(&led2);
	}
}

_attribute_ram_code_sec_ void timer1_irq_handler(void)
{
	if (timer_get_irq_status(TMR_STA_TMR1)) {
		timer_clr_irq_status(TMR_STA_TMR1);
#if (TIMER_MODE == TIMER_GPIO_TRIGGER_MODE)
		timer1_irq_cnt++;
#elif (TIMER_MODE == TIMER_GPIO_WIDTH_MODE)
		timer1_gpio_width = timer1_get_gpio_width();
		timer1_set_tick(0);
#endif
		gpio_pin_toggle_dt(&tim1_trig);
		gpio_pin_toggle_dt(&led3);
	}
}

static void app_init_leds(void)
{
	/* Initialize the GPIO pin for LED1 */
	if (!gpio_is_ready_dt(&led1)) {
		return;
	}
	if (gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) < 0) {
		return;
	}
	/* Initialize the GPIO pin for LED2 */
	if (!gpio_is_ready_dt(&led2)) {
		return;
	}
	if (gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE) < 0) {
		return;
	}
	/* Initialize the GPIO pin for LED3 */
	if (!gpio_is_ready_dt(&led3)) {
		return;
	}
	if (gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE) < 0) {
		return;
	}
	/* Initialize the GPIO pin for LED4 */
	if (!gpio_is_ready_dt(&led4)) {
		return;
	}
	if (gpio_pin_configure_dt(&led4, GPIO_OUTPUT_ACTIVE) < 0) {
		return;
	}
}

static void app_init_tim_gpio(void)
{
	/* Initialize the GPIO pins for TIM0 */
	if (!gpio_is_ready_dt(&tim0_trig)) {
		return;
	}
	if (gpio_pin_configure_dt(&tim0_trig, GPIO_OUTPUT_INACTIVE) < 0) {
		return;
	}
	if (!gpio_is_ready_dt(&tim0_capt)) {
		return;
	}
	if (gpio_pin_configure_dt(&tim0_capt, GPIO_INPUT | GPIO_PULL_DOWN) < 0) {
		return;
	}
	/* Initialize the GPIO pin for TIM1 */
	if (!gpio_is_ready_dt(&tim1_trig)) {
		return;
	}
	if (gpio_pin_configure_dt(&tim1_trig, GPIO_OUTPUT_INACTIVE) < 0) {
		return;
	}
	if (!gpio_is_ready_dt(&tim1_capt)) {
		return;
	}
	if (gpio_pin_configure_dt(&tim1_capt, GPIO_INPUT | GPIO_PULL_DOWN) < 0) {
		return;
	}
}

static void app_init_tim(void)
{
#if (TIMER_MODE == TIMER_GPIO_TRIGGER_MODE)
	timer0_irq_cnt = 0;
	timer1_irq_cnt = 0;

	IRQ_CONNECT(TIMER0_IRQ_NUM, 2, timer0_irq_handler, NULL, 0);
	IRQ_CONNECT(TIMER1_IRQ_NUM, 2, timer1_irq_handler, NULL, 0);

	core_interrupt_enable();

	riscv_plic_set_priority(IRQ4_TIMER0, 2);
	riscv_plic_irq_enable(IRQ4_TIMER0);

	riscv_plic_set_priority(IRQ3_TIMER1, 2);
	riscv_plic_irq_enable(IRQ3_TIMER1);

	/* timer0 POL_RISING tim0_capt PIN link */
	timer_gpio_init(TIMER0, GET_TIMER_CAPT_PIN(tim0_capt), POL_RISING);
	timer_set_init_tick(TIMER0, 0);
	timer_set_cap_tick(TIMER0, TIMER_GPIO_TRIGGER_TICK);
	timer_set_mode(TIMER0, TIMER_MODE_GPIO_TRIGGER);
	timer_start(TIMER0);

	/* timer1 POL_RISING tim1_capt PIN link */
	timer_gpio_init(TIMER1, GET_TIMER_CAPT_PIN(tim1_capt), POL_RISING);
	timer_set_init_tick(TIMER1, 0);
	timer_set_cap_tick(TIMER1, TIMER_GPIO_TRIGGER_TICK);
	timer_set_mode(TIMER1, TIMER_MODE_GPIO_TRIGGER);
	timer_start(TIMER1);

#elif (TIMER_MODE == TIMER_GPIO_WIDTH_MODE)
	timer0_gpio_width = 0;
	timer1_gpio_width = 0;

	IRQ_CONNECT(TIMER0_IRQ_NUM, 2, timer0_irq_handler, NULL, 0);
	IRQ_CONNECT(TIMER1_IRQ_NUM, 2, timer1_irq_handler, NULL, 0);

	core_interrupt_enable();

	riscv_plic_set_priority(IRQ4_TIMER0, 2);
	riscv_plic_irq_enable(IRQ4_TIMER0);

	riscv_plic_set_priority(IRQ3_TIMER1, 2);
	riscv_plic_irq_enable(IRQ3_TIMER1);

	/* timer0 POL_FALLING tim0_capt PIN link */
	gpio_pin_set_dt(&tim0_trig, 1);
	k_msleep(50);
	timer_gpio_init(TIMER0, GET_TIMER_CAPT_PIN(tim0_capt), POL_FALLING);
	timer_set_init_tick(TIMER0, 0);
	timer_set_cap_tick(TIMER0, 0);
	timer_set_mode(TIMER0, TIMER_MODE_GPIO_WIDTH);
	timer_start(TIMER0);
	gpio_pin_set_dt(&tim0_trig, 0);
	k_msleep(250);
	gpio_pin_set_dt(&tim0_trig, 1);

	/* timer1 POL_RISING tim1_capt PIN link */
	gpio_pin_set_dt(&tim1_trig, 0);
	k_msleep(50);
	timer_gpio_init(TIMER1, GET_TIMER_CAPT_PIN(tim1_capt), POL_RISING);
	timer_set_init_tick(TIMER1, 0);
	timer_set_cap_tick(TIMER1, 0);
	timer_set_mode(TIMER1, TIMER_MODE_GPIO_WIDTH);
	timer_start(TIMER1);
	gpio_pin_set_dt(&tim1_trig, 1);
	k_msleep(250);
	gpio_pin_set_dt(&tim1_trig, 0);

#elif (TIMER_MODE == TIMER_TICK_MODE)
	/* Timer0 */
	timer_set_init_tick(TIMER0, 0);
	timer_set_cap_tick(TIMER0, 0);
	timer_set_mode(TIMER0, TIMER_MODE_TICK); /* cpu clock tick. */
	timer_start(TIMER0);
	/* Timer1 */
	timer_set_init_tick(TIMER1, 0);
	timer_set_cap_tick(TIMER1, 0);
	timer_set_mode(TIMER1, TIMER_MODE_TICK); /* cpu clock tick. */
	timer_start(TIMER1);
#endif
}

void main_loop(void)
{
#if (TIMER_MODE == TIMER_GPIO_TRIGGER_MODE)
	printk("timer0_irq_cnt: %d\n", timer0_irq_cnt);
	printk("timer1_irq_cnt: %d\n", timer1_irq_cnt);
#elif (TIMER_MODE == TIMER_GPIO_WIDTH_MODE)
	printk("timer0_gpio_width: %d\n", timer0_gpio_width);
	printk("timer1_gpio_width: %d\n", timer1_gpio_width);
#elif (TIMER_MODE == TIMER_TICK_MODE)
	if (timer0_get_tick() > 500 * sys_clk.pclk * 1000) {
		timer0_set_tick(0);
		gpio_pin_toggle_dt(&led2);
		gpio_pin_toggle_dt(&led3);
	}
	if (timer1_get_tick() > 500 * sys_clk.pclk * 1000) {
		timer1_set_tick(0);
		gpio_pin_toggle_dt(&led4);
	}
#endif

#if (TIMER_MODE != TIMER_TICK_MODE)
	k_msleep(1000);
	gpio_pin_toggle_dt(&led1);
#endif
}

int main(void)
{
	app_init_leds();
	app_init_tim_gpio();
	app_init_tim();

	printk("TIMER App starts!\n");
#if (TIMER_MODE == TIMER_GPIO_TRIGGER_MODE)
	printk("Timer MODE is: TIMER_GPIO_TRIGGER_MODE\n");
#elif (TIMER_MODE == TIMER_GPIO_WIDTH_MODE)
	printk("Timer MODE is: TIMER_GPIO_WIDTH_MODE\n");
#elif (TIMER_MODE == TIMER_TICK_MODE)
	printk("Timer MODE is: TIMER_TICK_MODE\n");
#endif

	while (1) {
		main_loop();
	}

	return 0;
}
