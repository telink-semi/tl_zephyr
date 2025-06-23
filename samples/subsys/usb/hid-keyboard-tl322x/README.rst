.. zephyr:code-sample:: usb-hid-keyboard
   :name: USB HID keyboard

   Implement a basic HID keyboard device.

Overview
********

This sample app demonstrates use of a USB Human Interface Device (HID) driver
by the Zephyr project. This very simple driver enumerates a board with a button
into a keyboard that at least one key is required and up to four can be used.
The first three keys are used for Key 0, Caps Lock and Num Lock. The fourth
key is for Left Shift modifier.

Requirements
************

This project requires an USB device driver, and there must has at least one
GPIO button in your board. There must be a :dtcompatible:`gpio-keys` group of buttons
or keys defined at the board level that can generate input events.

The example can use up to three LEDs, configured via the devicetree alias such
as ``led0``, to indicate the state of the keyboard LEDs.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the tl3228x board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/hid-keyboard
   :board: tl3228x
   :goals: build
   :compact:


Programming Guide
*****************

Handling SET REPORT for USB HID Keyboard LED Control in Zephyr Version 3.3

1. Implement the SET REPORT Callback Function
=============================================
To handle LED state changes initiated by the host, augment the ``hid_on_set_report``
function in ``zephyr/subsys/usb/device/class/hid/core.c`` with callback logic.
This function processes incoming SET REPORT requests and delegates device-specific
handling to registered operations.

.. code-block:: c

    static int hid_on_set_report(struct hid_device_info *dev_data,
                                 struct usb_setup_packet *setup, int32_t *len,
                                 uint8_t **data)
    {
        LOG_DBG("Set Report callback");

        /* Implementation validated exclusively on Telink TL322x SoC */
        const struct hid_ops *ops = dev_data->ops;
        if (ops->set_report != NULL) {
            return ops->set_report(dev_data, setup, len, data);
        }

        return -ENOTSUP;
    }

2. Register the Callback in Device Operations
=============================================
In the application's ``main.c``, define a ``struct hid_ops`` structure and invoke
``usb_hid_register_device`` to register the callback function. This associates
keyboard-specific LED control logic with the HID subsystem.

.. code-block:: c

    /* Define keyboard-specific HID operation set */
    struct hid_ops kbd_ops = {
        .set_report = kbd_set_report,    /* LED control handler implementation */
        ...
    };

    usb_hid_register_device(hid_dev, hid_report_desc, sizeof(hid_report_desc), &kbd_ops);

- ``kbd_set_report``: Keyboard-specific function responsible for processing LED state data.

3. Implement the Device-Specific LED Control Function
=====================================================
The following function, implemented in ``main.c``, processes HID output reports to
control keyboard LEDs based on data received from the host.

.. code-block:: c

    int kbd_set_report(const struct device *dev, struct usb_setup_packet *setup,
                       int32_t *len, uint8_t **data)
    {
        /* Control Num Lock LED using corresponding bit in report data */
        gpio_pin_set(led1.port, led1.pin, (** data & HID_KBD_LED_NUM_LOCK) != 0);

        /* Control Caps Lock LED using corresponding bit in report data */
        gpio_pin_set(led2.port, led2.pin, (**data & HID_KBD_LED_CAPS_LOCK) != 0);

        return 0;
    }

4. Conclusion
=============
This implementation in the hid-keyboard sample has been validated exclusively for
Zephyr version 3.3. In subsequent versions of Zephyr, the usage methods of related
APIs may be modified. For adaptation to newer versions, further technical investigation
is advised.

NOTE:
=====
Since Zephyr version 3.7, a revised SET REPORT mechanism has introduced
via the USBD HID Device API. A new structure replaces ``hid_ops`` in
``zephyr/include/zephyr/usb/class/usbd_hid.h``:

.. code-block:: c

    struct hid_device_ops {
        void (*iface_ready)(const struct device *dev, const bool ready);

        int (*get_report)(const struct device *dev,
                          const uint8_t type, const uint8_t id,
                          const uint16_t len, uint8_t *const buf);

        int (*set_report)(const struct device *dev,
                          const uint8_t type, const uint8_t id,
                          const uint16_t len, const uint8_t *const buf);

        void (*set_idle)(const struct device *dev,
                         const uint8_t id, const uint32_t duration);

        uint32_t (*get_idle)(const struct device *dev, const uint8_t id);

        void (*set_protocol)(const struct device *dev, const uint8_t proto);

        void (*input_report_done)(const struct device *dev,
                                  const uint8_t *const report);

        void (*output_report)(const struct device *dev, const uint16_t len,
                              const uint8_t *const buf);

        void (*sof)(const struct device *dev);
    };

A new handler function replaces ``hid_on_set_report`` in ``zephyr/subsys/usb/device_next/class/usbd_hid.c``:

.. code-block:: c

    static int handle_set_report(const struct device *dev,
                                 const struct usb_setup_packet *const setup,
                                 const struct net_buf *const buf)
    {
        const uint8_t type = HID_GET_REPORT_TYPE(setup->wValue);
        const uint8_t id = HID_GET_REPORT_ID(setup->wValue);
        struct hid_device_data *const ddata = dev->data;
        const struct hid_device_ops *ops = ddata->ops;

        if (ops->set_report == NULL) {
            errno = -ENOTSUP;
            LOG_DBG("Set Report functionality not supported");
            return 0;
        }

        switch (type) {
        case HID_REPORT_TYPE_INPUT:
            LOG_DBG("Processing Set Report: Input Report ID %u", id);
            errno = ops->set_report(dev, type, id, buf->len, buf->data);
            break;
        case HID_REPORT_TYPE_OUTPUT:
            LOG_DBG("Processing Set Report: Output Report ID %u", id);
            errno = ops->set_report(dev, type, id, buf->len, buf->data);
            break;
        case HID_REPORT_TYPE_FEATURE:
            LOG_DBG("Processing Set Report: Feature Report ID %u", id);
            errno = ops->set_report(dev, type, id, buf->len, buf->data);
            break;
        default:
            errno = -ENOTSUP;
            break;
        }

        return 0;
    }
