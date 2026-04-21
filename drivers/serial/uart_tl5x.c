/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#define DT_DRV_COMPAT telink_tl5x_uart

struct uart_tl5x_data {
	struct uart_config cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

struct uart_tl5x_config {
	const struct pinctrl_dev_config *pcfg;
	uint32_t uart_addr;
	uint32_t baud_rate;
	void (*pirq_connect)(void);
	bool hw_flow_control;
};

static int uart_tl5x_driver_init(const struct device *dev)
{
	const struct uart_tl5x_config *cfg = dev->config;
	struct uart_tl5x_data *data = dev->data;
	int status;

	status = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		return status;
	}

	data->cfg.baudrate = cfg->baud_rate;
	data->cfg.parity = UART_CFG_PARITY_NONE;
	data->cfg.stop_bits = UART_CFG_STOP_BITS_1;
	data->cfg.data_bits = UART_CFG_DATA_BITS_8;
	data->cfg.flow_ctrl = cfg->hw_flow_control ?
			      UART_CFG_FLOW_CTRL_RTS_CTS :
			      UART_CFG_FLOW_CTRL_NONE;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->pirq_connect();
#endif

	return 0;
}

static void uart_tl5x_poll_out(const struct device *dev, uint8_t c)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(c);
}

static int uart_tl5x_poll_in(const struct device *dev, unsigned char *c)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(c);

	return -1;
}

static int uart_tl5x_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_tl5x_configure(const struct device *dev,
			       const struct uart_config *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

static int uart_tl5x_config_get(const struct device *dev,
				struct uart_config *cfg)
{
	struct uart_tl5x_data *data = dev->data;

	*cfg = data->cfg;

	return 0;
}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static void uart_tl5x_irq_handler(const struct device *dev)
{
	struct uart_tl5x_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, data->cb_data);
	}
}

static int uart_tl5x_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data,
			       int size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_data);
	ARG_UNUSED(size);

	return 0;
}

static int uart_tl5x_fifo_read(const struct device *dev,
			       uint8_t *rx_data,
			       const int size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_data);
	ARG_UNUSED(size);

	return 0;
}

static void uart_tl5x_irq_tx_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_tl5x_irq_tx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_tl5x_irq_tx_ready(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int uart_tl5x_irq_tx_complete(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void uart_tl5x_irq_rx_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_tl5x_irq_rx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_tl5x_irq_rx_ready(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void uart_tl5x_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_tl5x_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_tl5x_irq_is_pending(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int uart_tl5x_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_tl5x_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_tl5x_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_tl5x_driver_api = {
	.poll_in = uart_tl5x_poll_in,
	.poll_out = uart_tl5x_poll_out,
	.err_check = uart_tl5x_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_tl5x_configure,
	.config_get = uart_tl5x_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_tl5x_fifo_fill,
	.fifo_read = uart_tl5x_fifo_read,
	.irq_tx_enable = uart_tl5x_irq_tx_enable,
	.irq_tx_disable = uart_tl5x_irq_tx_disable,
	.irq_tx_ready = uart_tl5x_irq_tx_ready,
	.irq_tx_complete = uart_tl5x_irq_tx_complete,
	.irq_rx_enable = uart_tl5x_irq_rx_enable,
	.irq_rx_disable = uart_tl5x_irq_rx_disable,
	.irq_rx_ready = uart_tl5x_irq_rx_ready,
	.irq_err_enable = uart_tl5x_irq_err_enable,
	.irq_err_disable = uart_tl5x_irq_err_disable,
	.irq_is_pending = uart_tl5x_irq_is_pending,
	.irq_update = uart_tl5x_irq_update,
	.irq_callback_set = uart_tl5x_irq_callback_set,
#endif
};

#define UART_TL5X_INIT(n)							    \
										    \
	static void uart_tl5x_irq_connect_##n(void);				    \
										    \
	PINCTRL_DT_INST_DEFINE(n);						    \
										    \
	static const struct uart_tl5x_config uart_tl5x_cfg_##n =		    \
	{									    \
		.uart_addr = DT_INST_REG_ADDR(n),				    \
		.baud_rate = DT_INST_PROP(n, current_speed),			    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			    \
		.pirq_connect = uart_tl5x_irq_connect_##n,			    \
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control)		    \
	};									    \
										    \
	static struct uart_tl5x_data uart_tl5x_data_##n;			    \
										    \
	DEVICE_DT_INST_DEFINE(n, uart_tl5x_driver_init,				    \
			      NULL,						    \
			      &uart_tl5x_data_##n,				    \
			      &uart_tl5x_cfg_##n,				    \
			      PRE_KERNEL_1,					    \
			      CONFIG_SERIAL_INIT_PRIORITY,			    \
			      (void *)&uart_tl5x_driver_api);			    \
										    \
	static void uart_tl5x_irq_connect_##n(void)				    \
	{									    \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		    \
			    uart_tl5x_irq_handler,				    \
			    DEVICE_DT_INST_GET(n), 0);				    \
		irq_enable(DT_INST_IRQN(n));					    \
	}

DT_INST_FOREACH_STATUS_OKAY(UART_TL5X_INIT)