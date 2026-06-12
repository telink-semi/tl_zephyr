/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/policy.h>

#include <string.h>

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define MSG_SIZE 32

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});

/**/
static struct gpio_callback button_cb_data;
/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static volatile int rx_buf_pos;


void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	static bool uart_rx_enabled;

	if (uart_rx_enabled) {
		uart_rx_enabled = false;
		uart_irq_rx_disable(uart_dev);
		/* enable pm */
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		printk("%s reception disabled\n", uart_dev->name);
	} else {
		/* disable pm */
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
        /* clear uart rx fifo */
		uint8_t c;

		while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		}
		rx_buf_pos = 0;
		uart_irq_rx_enable(uart_dev);
		uart_rx_enabled = true;
		printk("%s reception enabled\n", uart_dev->name);
	}
}

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if (c == '\n' || c == '\r') {
			if (rx_buf_pos > 0) {
				/* terminate string */
				rx_buf[rx_buf_pos] = '\0';

				/* if queue is full, message is silently dropped */
				k_msgq_put(&uart_msgq, rx_buf, K_NO_WAIT);

				/* reset the buffer (it was copied to the msgq) */
				rx_buf_pos = 0;
			}
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

int main(void)
{
#if 0
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
	char tx_buf[MSG_SIZE];

	if (!button.port) {
		printk("No \"sw0\" described in .dts");
		return 0;
	}

	if (!gpio_is_ready_dt(&button)) {
		printk("Buttom device not found!");;
		return 0;
	}

	int ret;

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return 0;
	}

	/* configure interrupt and callback to receive data */
	ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}

	printk("Hello! I'm your echo bot.\r\n");
	printk("%s reception disabled\n", uart_dev->name);

	/* indefinitely wait for input from the user */
	while (k_msgq_get(&uart_msgq, tx_buf, K_FOREVER) == 0) {
		print_uart("Echo: ");
		print_uart(tx_buf);
		print_uart("\r\n");
		printk("received and sent echo!\n");
	}
	return 0;
}
