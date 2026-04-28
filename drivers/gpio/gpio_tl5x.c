/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"
#include "gpio.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>

#include <string.h>

#define DT_DRV_COMPAT telink_tl5x_gpio

struct gpio_tl5x_t {
	uint8_t input;
	uint8_t ie;
	uint8_t oe;
	uint8_t polarity;
	uint8_t pin1;
	uint8_t pin2;
	uint8_t func;
	uint8_t toggle;
	uint8_t out_set;
	uint8_t out_clear;
};

#define GET_GPIO(dev)           ((volatile struct gpio_tl5x_t *) \
				 ((const struct gpio_tl5x_config *)dev->config)->gpio_base)

#define GET_PORT_NUM(gpio)      ((uint8_t)(((uint32_t)gpio - \
					   DT_REG_ADDR(DT_NODELABEL(gpioa))) / \
					   DT_REG_SIZE(DT_NODELABEL(gpioa))))

#define IS_PORT_C(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpioc)))
#define IS_PORT_E(gpio)         ((uint32_t)gpio == DT_REG_ADDR(DT_NODELABEL(gpioe)))

#define PIN_NUM_MAX ((uint8_t)7u)

#define GPIO_SET_LOW_LEVEL(gpio, pin)  WRITE_BIT(gpio->out_clear, pin, 1)
#define GPIO_SET_HIGH_LEVEL(gpio, pin) WRITE_BIT(gpio->out_set, pin, 1)

#define GPIO_PIN_UP_DOWN_FLOAT   ((uint8_t)0u)
#define GPIO_PIN_PULLUP_1M       ((uint8_t)1u)
#define GPIO_PIN_PULLDOWN_100K   ((uint8_t)2u)
#define GPIO_PIN_PULLUP_20K      ((uint8_t)3u)

#define INTR_RISING_EDGE         ((uint8_t)0u)
#define INTR_FALLING_EDGE        ((uint8_t)1u)

#define IRQ_GPIO0                ((uint8_t)34u)
#define IRQ_GPIO1                ((uint8_t)35u)
#define IRQ_GPIO2                ((uint8_t)36u)
#define IRQ_GPIO3                ((uint8_t)37u)
#define IRQ_GPIO4                ((uint8_t)38u)
#define IRQ_GPIO5                ((uint8_t)39u)
#define IRQ_GPIO6                ((uint8_t)40u)
#define IRQ_GPIO7                ((uint8_t)41u)

#define GET_IRQ_NUM(dev)         (irq_from_level_2(((const struct gpio_tl5x_config *)dev->config)->irq_num))
#define GET_LV2_IRQ_NUM(irq)     (IRQ_TO_L2(irq) + CONFIG_2ND_LVL_INTR_00_OFFSET)
#define GET_IRQ_PRIORITY(dev)    (((const struct gpio_tl5x_config *)dev->config)->irq_priority)

#define GET_GPIO_BASE_ADDR(gpio) ((uint32_t)(gpio) - (GET_PORT_NUM(gpio) * 0x10))

#define REG_GPIO_IRQ_CTRL(gpio)     (*(volatile uint8_t *)(GET_GPIO_BASE_ADDR(gpio) + 0x90))
#define REG_GPIO_IRQ_PAD_MASK(gpio) (*(volatile uint8_t *)(GET_GPIO_BASE_ADDR(gpio) + 0x91))
#define REG_GPIO_IRQ_LVL(gpio)      (*(volatile uint8_t *)(GET_GPIO_BASE_ADDR(gpio) + 0x92))
#define REG_GPIO_IRQ_STATUS(gpio)   (*(volatile uint8_t *)(GET_GPIO_BASE_ADDR(gpio) + 0x93))
#define REG_GPIO_IRQ_EN(gpio, irq)  (*(volatile uint8_t *)(GET_GPIO_BASE_ADDR(gpio) + \
						    0x94 + (GET_PORT_NUM(gpio) << 3) + (irq)))

#define areg_gpio_pc_ie  0xa4

struct gpio_tl5x_pin_irq_config {
	gpio_port_value_t pin_last_value;
	gpio_port_value_t irq_en_rising;
	gpio_port_value_t irq_en_falling;
	gpio_port_value_t irq_en_both;
};

struct gpio_tl5x_config {
	struct gpio_driver_config common;
	uint32_t gpio_base;
	uint32_t irq_num;
	uint8_t irq_priority;
	struct gpio_tl5x_pin_irq_config *pin_irq_state;
	void (*pirq_connect)(void);
};

struct gpio_tl5x_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static inline void gpio_tl5x_irq_en_set(const struct device *dev, gpio_pin_t pin)
{
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);
	uint8_t irq_idx = irq - IRQ_GPIO0;

	BM_SET(REG_GPIO_IRQ_EN(gpio, irq_idx), BIT(pin));
}

static inline void gpio_tl5x_irq_en_clr(const struct device *dev, gpio_pin_t pin)
{
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);
	uint8_t irq_idx = irq - IRQ_GPIO0;

	BM_CLR(REG_GPIO_IRQ_EN(gpio, irq_idx), BIT(pin));
}
static inline uint8_t gpio_tl5x_irq_en_get(const struct device *dev)
{
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);
	uint8_t irq_idx = irq - IRQ_GPIO0;

	return REG_GPIO_IRQ_EN(gpio, irq_idx);
}

/* Clear IRQ Status bit */
static inline void gpio_tl5x_irq_status_clr(const struct device *dev)
{
	uint8_t irq = GET_IRQ_NUM(dev);
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);
	uint8_t irq_idx = irq - IRQ_GPIO0;
	gpio_irq_e status = BIT(irq_idx);
 
	REG_GPIO_IRQ_STATUS(gpio) = status;
}

static void gpio_tl5x_irq_set(const struct device *dev, gpio_pin_t pin,
		      uint8_t trigger_type)
{
	uint8_t irq_lvl = 0;
	uint8_t irq_mask = 0;
	uint8_t irq_num = GET_IRQ_NUM(dev);
	uint8_t irq_priority = GET_IRQ_PRIORITY(dev);
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);
	uint8_t irq_idx = irq_num - IRQ_GPIO0;

	gpio_tl5x_irq_status_clr(dev);

	irq_lvl = BIT(irq_idx);
	irq_mask = BIT(irq_idx);

	switch (trigger_type) {
	case INTR_RISING_EDGE:
		BM_CLR(gpio->polarity, BIT(pin));
		BM_CLR(REG_GPIO_IRQ_LVL(gpio), irq_lvl);
		break;

	case INTR_FALLING_EDGE:
		BM_SET(gpio->polarity, BIT(pin));
		BM_CLR(REG_GPIO_IRQ_LVL(gpio), irq_lvl);
		break;
	}

	if (irq_num == IRQ_GPIO0) {
		REG_GPIO_IRQ_CTRL(gpio) |= FLD_IRQ_EN;
	}

	BM_SET(REG_GPIO_IRQ_PAD_MASK(gpio), irq_mask);

	gpio_tl5x_irq_en_set(dev, pin);

	riscv_plic_irq_enable(GET_LV2_IRQ_NUM(irq_num));
	riscv_plic_set_priority(GET_LV2_IRQ_NUM(irq_num), irq_priority);
}

static void gpio_tl5x_up_down_res_set(volatile struct gpio_tl5x_t *gpio,
				     gpio_pin_t pin,
				     uint8_t up_down_res)
{
	uint8_t val;
	uint8_t mask;
	uint8_t analog_reg;
	uint8_t port_num = GET_PORT_NUM(gpio);

	if (IS_PORT_E(gpio)) {
		return;
	}

	pin = BIT(pin);
	val = up_down_res & 0x03;

	analog_reg = 0x17 + (port_num << 1) + ((pin & 0xf0) ? 1 : 0);

	if (pin & 0x11) {
		val = val << 0;
		mask = 0xfc;
	} else if (pin & 0x22) {
		val = val << 2;
		mask = 0xf3;
	} else if (pin & 0x44) {
		val = val << 4;
		mask = 0xcf;
	} else if (pin & 0x88) {
		val = val << 6;
		mask = 0x3f;
	} else {
		return;
	}

	analog_write_reg8(analog_reg, (analog_read_reg8(analog_reg) & mask) | val);
}

static void gpio_tl5x_config_up_down_res(volatile struct gpio_tl5x_t *gpio,
					gpio_pin_t pin,
					gpio_flags_t flags)
{
	if ((flags & GPIO_PULL_UP) != 0) {
		gpio_tl5x_up_down_res_set(gpio, pin, GPIO_PIN_PULLUP_20K);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		gpio_tl5x_up_down_res_set(gpio, pin, GPIO_PIN_PULLDOWN_100K);
	} else {
		gpio_tl5x_up_down_res_set(gpio, pin, GPIO_PIN_UP_DOWN_FLOAT);
	}
}

static void gpio_tl5x_config_in_out(volatile struct gpio_tl5x_t *gpio,
				   gpio_pin_t pin,
				   gpio_flags_t flags)
{
	uint8_t ie_addr = 0;

	if (IS_PORT_C(gpio)) {
		ie_addr = areg_gpio_pc_ie;
	}

	WRITE_BIT(gpio->oe, pin, ~flags & GPIO_OUTPUT);

	if (ie_addr != 0) {
		if (flags & GPIO_INPUT) {
			analog_write_reg8(ie_addr, analog_read_reg8(ie_addr) | BIT(pin));
		} else {
			analog_write_reg8(ie_addr, analog_read_reg8(ie_addr) & (~BIT(pin)));
		}
	} else {
		WRITE_BIT(gpio->ie, pin, flags & GPIO_INPUT);
	}
}

static int gpio_tl5x_init(const struct device *dev)
{
	const struct gpio_tl5x_config *cfg = dev->config;

	cfg->pirq_connect();

	return 0;
}

static int gpio_tl5x_pin_configure(const struct device *dev,
				  gpio_pin_t pin,
				  gpio_flags_t flags)
{
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);

	printk("gpio_tl5x_pin_configure: pin %d, flags 0x%x\n", pin, flags);

	if (pin > PIN_NUM_MAX) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) && (flags & GPIO_INPUT)) {
		return -ENOTSUP;
	}

	if (IS_PORT_E(gpio) && (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN))) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		printk("gpio_tl5x_pin_configure: pin %d is output high\n", pin);
		GPIO_SET_HIGH_LEVEL(gpio, pin);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		printk("gpio_tl5x_pin_configure: pin %d is output low\n", pin);
		GPIO_SET_LOW_LEVEL(gpio, pin);
	}

	printk("gpio_tl5x_pin_configure: pin %d is configured\n", pin);

	WRITE_BIT(gpio->func, pin, 1);

	gpio_tl5x_config_up_down_res(gpio, pin, flags);

	gpio_tl5x_config_in_out(gpio, pin, flags);

	

	return 0;
}

static int gpio_tl5x_port_get_raw(const struct device *dev,
				 gpio_port_value_t *value)
{
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);

	*value = gpio->input;

	return 0;
}

static int gpio_tl5x_port_set_masked_raw(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);

	gpio->out_clear = mask;
	gpio->out_set = (value & mask);

	return 0;
}

static int gpio_tl5x_port_set_bits_raw(const struct device *dev,
				      gpio_port_pins_t mask)
{
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);

	gpio->out_set = mask;

	return 0;
}

static int gpio_tl5x_port_clear_bits_raw(const struct device *dev,
					gpio_port_pins_t mask)
{
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);

	gpio->out_clear = mask;

	return 0;
}

static int gpio_tl5x_port_toggle_bits(const struct device *dev,
				     gpio_port_pins_t mask)
{
	volatile struct gpio_tl5x_t *gpio = GET_GPIO(dev);
	uint8_t bits = (mask & 0xff);

	gpio->toggle = bits;

	return 0;
}

static void gpio_tl5x_irq_handler(const struct device *dev)
{
	struct gpio_tl5x_data *data = dev->data;
	const struct gpio_tl5x_config *cfg = dev->config;
	uint8_t irq = GET_IRQ_NUM(dev);

	gpio_port_value_t current_pins       = GET_GPIO(dev)->input;
	gpio_port_value_t changed_pins0      = (cfg->pin_irq_state->pin_last_value ^ current_pins)
		& (~current_pins);
	gpio_port_value_t changed_pins1      = (cfg->pin_irq_state->pin_last_value ^ current_pins)
		& current_pins;
	gpio_port_value_t fired_irqs_rising  = changed_pins1 & cfg->pin_irq_state->irq_en_rising;
	gpio_port_value_t fired_irqs_falling = changed_pins0 & cfg->pin_irq_state->irq_en_falling;
	gpio_port_value_t fired_irqs_both    = (changed_pins0 | changed_pins1)
		& cfg->pin_irq_state->irq_en_both;
	gpio_port_value_t fired_irqs         = fired_irqs_rising | fired_irqs_falling
		| fired_irqs_both;

	cfg->pin_irq_state->pin_last_value = current_pins;
	GET_GPIO(dev)->polarity ^= (changed_pins0 | changed_pins1);

	gpio_tl5x_irq_status_clr(dev);
	gpio_fire_callbacks(&data->callbacks, dev, fired_irqs);
}

static int gpio_tl5x_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_tl5x_config *cfg = dev->config;
	int ret_status = 0;
	bool current_pin_value = ((GET_GPIO(dev)->input) >> pin) & 0x0001;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		gpio_tl5x_irq_en_clr(dev, pin);
		BM_CLR(cfg->pin_irq_state->irq_en_rising, BIT(pin));
		BM_CLR(cfg->pin_irq_state->irq_en_falling, BIT(pin));
		BM_CLR(cfg->pin_irq_state->irq_en_both, BIT(pin));
		if (!cfg->pin_irq_state->irq_en_rising && !cfg->pin_irq_state->irq_en_falling &&
		    !cfg->pin_irq_state->irq_en_both) {
			riscv_plic_irq_disable(cfg->irq_num);
		}
		break;

	case GPIO_INT_MODE_EDGE:
		if (trig == GPIO_INT_TRIG_HIGH) {
			BM_SET(cfg->pin_irq_state->irq_en_rising, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_falling, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_both, BIT(pin));
		} else if (trig == GPIO_INT_TRIG_LOW) {
			BM_SET(cfg->pin_irq_state->irq_en_falling, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_rising, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_both, BIT(pin));
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			BM_SET(cfg->pin_irq_state->irq_en_both, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_rising, BIT(pin));
			BM_CLR(cfg->pin_irq_state->irq_en_falling, BIT(pin));
		} else {
			ret_status = -ENOTSUP;
		}

		if (current_pin_value) {
			gpio_tl5x_irq_set(dev, pin, INTR_FALLING_EDGE);
		} else {
			gpio_tl5x_irq_set(dev, pin, INTR_RISING_EDGE);
		}

		if (ret_status == 0) {
			current_pin_value ? BM_SET(cfg->pin_irq_state->pin_last_value, BIT(pin)) :
				BM_CLR(cfg->pin_irq_state->pin_last_value, BIT(pin));
		}
		break;

	case GPIO_INT_MODE_LEVEL:
	default:
		ret_status = -ENOTSUP;
		break;
	}

	return ret_status;
}

static int gpio_tl5x_manage_callback(const struct device *dev,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_tl5x_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static const struct gpio_driver_api gpio_tl5x_driver_api = {
	.pin_configure = gpio_tl5x_pin_configure,
	.port_get_raw = gpio_tl5x_port_get_raw,
	.port_set_masked_raw = gpio_tl5x_port_set_masked_raw,
	.port_set_bits_raw = gpio_tl5x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tl5x_port_clear_bits_raw,
	.port_toggle_bits = gpio_tl5x_port_toggle_bits,
	.pin_interrupt_configure = gpio_tl5x_pin_interrupt_configure,
	.manage_callback = gpio_tl5x_manage_callback
};

#define IS_INST_IRQ_EN(inst)    (DT_NUM_IRQS(DT_DRV_INST(inst)) >= 1)

#define GPIO_TL5X_IRQ_CONNECT(n)					    \
	static void gpio_tl5x_irq_connect_##n(void)			    \
	{								    \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	    \
			    gpio_tl5x_irq_handler,			    \
			    DEVICE_DT_INST_GET(n), 0);			    \
		irq_enable(DT_INST_IRQN(n));				    \
	}

#define GPIO_TL5X_INIT(n)					    \
	static struct gpio_tl5x_pin_irq_config gpio_tl5x_pin_irq_state_##n; \
	GPIO_TL5X_IRQ_CONNECT(n)					    \
	static const struct gpio_tl5x_config gpio_tl5x_config_##n = {	    \
		.common = {						    \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n)  \
		},							    \
		.gpio_base = DT_INST_REG_ADDR(n),			    \
		.irq_num = DT_INST_IRQN(n),				    \
		.irq_priority = DT_INST_IRQ(n, priority),		    \
		.pin_irq_state = &gpio_tl5x_pin_irq_state_##n,		    \
		.pirq_connect = gpio_tl5x_irq_connect_##n		    \
	};								    \
	static struct gpio_tl5x_data gpio_tl5x_data_##n;		    \
	DEVICE_DT_INST_DEFINE(n, gpio_tl5x_init,				    \
			      NULL,					    \
			      &gpio_tl5x_data_##n,				    \
			      &gpio_tl5x_config_##n,				    \
			      PRE_KERNEL_1,				    \
			      CONFIG_GPIO_INIT_PRIORITY,			    \
			      &gpio_tl5x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TL5X_INIT)