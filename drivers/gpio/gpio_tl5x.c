/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/pm/device.h>

#include <string.h>

/* Driver dts compatibility: telink,tl5x_gpio */
#define DT_DRV_COMPAT telink_tl5x_gpio

struct gpio_tl5x_config {
	struct gpio_driver_config common;
	uint32_t gpio_base;
	uint32_t irq_num;
	uint8_t irq_priority;
	void (*pirq_connect)(void);
};

struct gpio_tl5x_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

/* Set IRQ Enable bit based on IRQ number */
static inline void gpio_tlx_irq_en_set(const struct device *dev, gpio_pin_t pin)
{


}

/* Clear IRQ Enable bit based on IRQ number */
static inline void gpio_tlx_irq_en_clr(const struct device *dev, gpio_pin_t pin)
{

}

static inline uint8_t gpio_tlx_irq_en_get(const struct device *dev)
{
    
}

/* Clear IRQ Status bit */
static inline void gpio_tlx_irq_status_clr(uint8_t irq)
{

}

/* Set pin's irq type */
void gpio_tlx_irq_set(const struct device *dev, gpio_pin_t pin,
		      uint8_t trigger_type)
{

}

/* Set pin's pull-up/down resistor */
static void gpio_tlx_up_down_res_set(volatile struct gpio_tlx_t *gpio,
				     gpio_pin_t pin,
				     uint8_t up_down_res)
{

}

/* Config Pin pull-up / pull-down resistors */
static void gpio_tlx_config_up_down_res(volatile struct gpio_tlx_t *gpio,
					gpio_pin_t pin,
					gpio_flags_t flags)
{

}

/* Config Pin In/Out direction */
static void gpio_tlx_config_in_out(volatile struct gpio_tlx_t *gpio,
				   gpio_pin_t pin,
				   gpio_flags_t flags)
{

}

/* GPIO driver initialization */
static int gpio_tlx_init(const struct device *dev)
{
	return 0;
}

/* API implementation: pin_configure */
static int gpio_tlx_pin_configure(const struct device *dev,
				  gpio_pin_t pin,
				  gpio_flags_t flags)
{

}

/* API implementation: port_get_raw */
static int gpio_tlx_port_get_raw(const struct device *dev,
				 gpio_port_value_t *value)
{

}

/* API implementation: port_set_masked_raw */
static int gpio_tlx_port_set_masked_raw(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{

}

/* API implementation: port_set_bits_raw */
static int gpio_tlx_port_set_bits_raw(const struct device *dev,
				      gpio_port_pins_t mask)
{

}

/* API implementation: port_clear_bits_raw */
static int gpio_tlx_port_clear_bits_raw(const struct device *dev,
					gpio_port_pins_t mask)
{
    
}

/* API implementation: port_toggle_bits */
static int gpio_tlx_port_toggle_bits(const struct device *dev,
				     gpio_port_pins_t mask)
{

}

/* API implementation: interrupts handler */
static void gpio_tlx_irq_handler(const struct device *dev)
{

}

/* API implementation: pin_interrupt_configure */
static int gpio_tlx_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{

}

/* API implementation: manage_callback */
static int gpio_tlx_manage_callback(const struct device *dev,
				    struct gpio_callback *callback,
				    bool set)
{

}

/* GPIO driver APIs structure */
static const struct gpio_driver_api gpio_tlx_driver_api = {
	.pin_configure = gpio_tlx_pin_configure,
	.port_get_raw = gpio_tlx_port_get_raw,
	.port_set_masked_raw = gpio_tlx_port_set_masked_raw,
	.port_set_bits_raw = gpio_tlx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tlx_port_clear_bits_raw,
	.port_toggle_bits = gpio_tlx_port_toggle_bits,
	.pin_interrupt_configure = gpio_tlx_pin_interrupt_configure,
	.manage_callback = gpio_tlx_manage_callback
};

#define GPIO_TL5X_INIT(n)						    \
	static void gpio_tl5x_irq_connect_##n(void);			    \
	static const struct gpio_tl5x_config gpio_tl5x_config_##n = {	    \
		.common = {						    \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n)  \
		},							    \
		.gpio_base = DT_INST_REG_ADDR(n),			    \
		.irq_num = DT_INST_IRQN(n),				    \
		.irq_priority = DT_INST_IRQ(n, priority),		    \
		.pirq_connect = gpio_tl5x_irq_connect_##n		    \
	};								    \
	static struct gpio_tl5x_data gpio_tl5x_data_##n;		    \
	DEVICE_DT_INST_DEFINE(n, gpio_tlx_init,				    \
			      NULL,					    \
			      &gpio_tl5x_data_##n,			    \
			      &gpio_tl5x_config_##n,			    \
			      PRE_KERNEL_1,				    \
			      CONFIG_GPIO_INIT_PRIORITY,		    \
			      &gpio_tlx_driver_api);			    \
	static void gpio_tl5x_irq_connect_##n(void)			    \
	{								    \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	    \
			    gpio_tlx_irq_handler,			    \
			    DEVICE_DT_INST_GET(n), 0);			    \
		irq_enable(DT_INST_IRQN(n));				    \
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_TL5X_INIT)