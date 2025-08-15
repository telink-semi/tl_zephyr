/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/*
 * Devicetree node identifiers for the buttons and LED this sample
 * supports.
 */
#define SW0_NODE  DT_ALIAS(sw0)
#define SW1_NODE  DT_ALIAS(sw1)
#define SW2_NODE  DT_ALIAS(sw2)
#define SW3_NODE  DT_ALIAS(sw3)
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

/*
 * Button sw0 and LED led0 are required.
 */
#if !DT_NODE_EXISTS(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#if !DT_NODE_EXISTS(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

/*
 * Helper macro for initializing a gpio_dt_spec from the devicetree
 * with fallback values when the nodes are missing.
 */
#define GPIO_SPEC(node_id) GPIO_DT_SPEC_GET_OR(node_id, gpios, {0})

/*
 * Create gpio_dt_spec structures from the devicetree.
 */
static const struct gpio_dt_spec sw0 = GPIO_SPEC(SW0_NODE),
								 sw1 = GPIO_SPEC(SW1_NODE),
								 sw2 = GPIO_SPEC(SW2_NODE),
								 sw3 = GPIO_SPEC(SW3_NODE),
								 led0 = GPIO_SPEC(LED0_NODE),
								 led1 = GPIO_SPEC(LED1_NODE),
								 led2 = GPIO_SPEC(LED2_NODE);

static const struct gpio_dt_spec leds[] = {
	led0,
	led1,
	led2
};

static K_SEM_DEFINE(evt_sem, 0, 1); /* starts off "not available" */
static K_SEM_DEFINE(usb_sem, 1, 1); /* starts off "available" */
static struct gpio_callback callback[4];
static enum usb_dc_status_code usb_status;
static const uint8_t hid_report_desc[] = HID_KEYBOARD_REPORT_DESC();

/*
 * Keyboard report structure.
 */
enum kbd_report_idx {
	KB_MOD_KEY = 0,
	KB_RESERVED,
	KB_KEY_CODE1,
	KB_KEY_CODE2,
	KB_KEY_CODE3,
	KB_KEY_CODE4,
	KB_KEY_CODE5,
	KB_KEY_CODE6,
	KB_REPORT_COUNT,
};

/*
 * Event FIFO
 */
K_FIFO_DEFINE(evt_fifo);

enum evt_t {
	GPIO_BUTTON_0 = 0x00,
	GPIO_BUTTON_1 = 0x01,
	GPIO_BUTTON_2 = 0x02,
	GPIO_BUTTON_3 = 0x03,
	HID_KBD_CLEAR = 0x04,
};

struct app_evt_t {
	sys_snode_t node;
	enum evt_t event_type;
};

#define FIFO_ELEM_MIN_SZ sizeof(struct app_evt_t)
#define FIFO_ELEM_MAX_SZ sizeof(struct app_evt_t)
#define FIFO_ELEM_COUNT  255
#define FIFO_ELEM_ALIGN  sizeof(unsigned int)

K_HEAP_DEFINE(event_elem_pool, FIFO_ELEM_MAX_SZ *FIFO_ELEM_COUNT + 256);

static inline void app_evt_free(struct app_evt_t *ev)
{
	k_heap_free(&event_elem_pool, ev);
}

static inline void app_evt_put(struct app_evt_t *ev)
{
	k_fifo_put(&evt_fifo, ev);
}

static inline struct app_evt_t *app_evt_get(void)
{
	return k_fifo_get(&evt_fifo, K_NO_WAIT);
}

static inline void app_evt_flush(void)
{
	struct app_evt_t *ev;

	do {
		ev = app_evt_get();
		if (ev) {
			app_evt_free(ev);
		}
	} while (ev != NULL);
}

static inline struct app_evt_t *app_evt_alloc(void)
{
	struct app_evt_t *ev;

	ev = k_heap_alloc(&event_elem_pool, sizeof(struct app_evt_t), K_NO_WAIT);
	if (ev == NULL) {
		LOG_ERR("APP event allocation failed!");
		app_evt_flush();

		ev = k_heap_alloc(&event_elem_pool, sizeof(struct app_evt_t), K_NO_WAIT);
		if (ev == NULL) {
			LOG_ERR("APP event memory corrupted.");
			__ASSERT_NO_MSG(0);
			return NULL;
		}
		return NULL;
	}

	return ev;
}

static void in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	k_sem_give(&usb_sem);
}

/* LED control handler implementation */
int kbd_set_report(const struct device *dev, struct usb_setup_packet *setup, int32_t *len,
			uint8_t **data)
{
	gpio_pin_set(led1.port, led1.pin, (**data & HID_KBD_LED_NUM_LOCK));
	gpio_pin_set(led2.port, led2.pin, (**data & HID_KBD_LED_CAPS_LOCK));

	return 0;
}

struct hid_ops kbd_ops = {
	.set_report = kbd_set_report,
	.int_in_ready = in_ready_cb,
};

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
}

/*
 * GPIO callback handler.
 */
static void btn0(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			LOG_DBG("btn0 wakeup_request");
			usb_wakeup_request();
			return;
		}
	}

	ev->event_type = GPIO_BUTTON_0, app_evt_put(ev);
	k_sem_give(&evt_sem);
}

static void btn1(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			LOG_DBG("btn1 wakeup_request");
			usb_wakeup_request();
			return;
		}
	}

	ev->event_type = GPIO_BUTTON_1, app_evt_put(ev);
	k_sem_give(&evt_sem);
}

static void btn2(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			LOG_DBG("btn2 wakeup_request");
			usb_wakeup_request();
			return;
		}
	}

	ev->event_type = GPIO_BUTTON_2, app_evt_put(ev);
	k_sem_give(&evt_sem);
}

static void btn3(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			LOG_DBG("btn3 wakeup_request");
			usb_wakeup_request();
			return;
		}
	}

	ev->event_type = GPIO_BUTTON_3, app_evt_put(ev);
	k_sem_give(&evt_sem);
}

static void clear_kbd_report(void)
{
	struct app_evt_t *new_evt = app_evt_alloc();

	new_evt->event_type = HID_KBD_CLEAR;
	app_evt_put(new_evt);
	k_sem_give(&evt_sem);
}

int callbacks_configure(const struct gpio_dt_spec *spec, gpio_callback_handler_t handler,
			struct gpio_callback *callback)
{
	const struct device *gpio = spec->port;
	gpio_pin_t pin = spec->pin;
	int ret;

	if (gpio == NULL) {
		/* Optional GPIO is missing. */
		return 0;
	}

	if (!device_is_ready(gpio)) {
		LOG_ERR("GPIO port %s is not ready", gpio->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure port %s pin %u, error: %d", gpio->name, pin, ret);
		return ret;
	}

	gpio_init_callback(callback, handler, BIT(pin));
	ret = gpio_add_callback(gpio, callback);
	if (ret < 0) {
		LOG_ERR("Failed to add the callback for port %s pin %u, "
			"error: %d",
			gpio->name, pin, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt for port %s pin %u, "
			"error: %d",
			gpio->name, pin, ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	int ret;
	uint8_t report[KB_REPORT_COUNT] = {0x00};
	const struct device *hid_dev;
	struct app_evt_t *ev;

	for (uint8_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!device_is_ready(leds[i].port)) {
			LOG_ERR("LED%d device %s is not ready", i, leds[i].port->name);
			return 0;
		}
		
		ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure LED%d pin, error: %d", i, ret);
			return 0;
		}
	}

	if (callbacks_configure(&sw0, &btn0, &callback[0])) {
		LOG_ERR("Failed configuring button 0 callback.");
		return 0;
	}
	if (callbacks_configure(&sw1, &btn1, &callback[1])) {
		LOG_ERR("Failed configuring button 1 callback.");
		return 0;
	}
	if (callbacks_configure(&sw2, &btn2, &callback[2])) {
		LOG_ERR("Failed configuring button 2 callback.");
		return 0;
	}
	if (callbacks_configure(&sw3, &btn3, &callback[3])) {
		LOG_ERR("Failed configuring button 3 callback.");
		return 0;
	}

	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return 0;
	}

	usb_hid_register_device(hid_dev, hid_report_desc, sizeof(hid_report_desc), &kbd_ops);
	usb_hid_init(hid_dev);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	while (true) {
		k_sem_take(&evt_sem, K_FOREVER);

		while ((ev = app_evt_get()) != NULL) {
			switch (ev->event_type) {
			case GPIO_BUTTON_0: {
				/* Press the Key 0 */
				report[KB_KEY_CODE1] = HID_KEY_0;
				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
				clear_kbd_report();
				break;
			}
			case GPIO_BUTTON_1: {
				/* Toggle Caps Lock */
				report[KB_KEY_CODE2] = HID_KEY_CAPSLOCK;
				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
				clear_kbd_report();
				break;
			}
			case GPIO_BUTTON_2: {
				/* Toggle Num Lock */
				report[KB_KEY_CODE3] = HID_KEY_NUMLOCK;
				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
				clear_kbd_report();
				break;
			}
			case GPIO_BUTTON_3: {
				/* Toggle Left Shift */
				report[KB_MOD_KEY] = HID_KBD_MODIFIER_LEFT_SHIFT;
				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
				clear_kbd_report();
				break;
			}
			case HID_KBD_CLEAR: {
				/* Clear keyboard report */
				for (int i = 0; i < KB_REPORT_COUNT; i++) {
					report[i] = 0x00;
				}
				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
				gpio_pin_toggle(led0.port, led0.pin);
				break;
			}
			default: {
				LOG_ERR("Unknown event to execute");
				break;
			} break;
			}
			app_evt_free(ev);
		}
	}
	return 0;
}
