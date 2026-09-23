/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  udc_tl322x.c
 * @brief Telink TL322X USB device controller (UDC) driver
 *
 * The driver implements the interface between the Telink TL322X USBD
 * peripheral driver (usb0hw) and the UDC API. The architecture follows
 * the Nordic udc_nrf.c driver: a dedicated thread consumes events queued
 * by the interrupt handler.
 */

#define DT_DRV_COMPAT telink_tl322x_usbd

#ifdef CONFIG_SOC_RISCV_TELINK_TL322X
#include "driver.h"
#endif

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>

#include <soc.h>

#include "udc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_tl322x, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define UDC_TLX_NUM_BIDIR_EP	DT_INST_PROP(0, num_bidir_endpoints)
#define UDC_TLX_MPS0		UDC_MPS0_64
#define UDC_TLX_EP0_SIZE	64

/* Total hardware endpoint FIFO size and split between RX and TX. */
#define EPS_BUFFER_TOTAL_SIZE	(8 * 1024)
#define EPS_BUFFER_OUT_SIZE	(0xFF)
#define EPS_BUFFER_IN_SIZE	(EPS_BUFFER_TOTAL_SIZE - EPS_BUFFER_OUT_SIZE)

/**
 * @brief Endpoint FIFO information.
 *
 * Tracks the available start address and remaining size of the endpoint
 * cache while configuring IN endpoint TX FIFOs.
 */
struct tlx_ep_fifo {
	uint8_t seg_addr;
	uint16_t remaining_size;
};

static struct tlx_ep_fifo eps_fifo;

/* Driver event types processed by the driver thread. */
enum udc_tlx_evt_type {
	UDC_TLX_EVT_RESET,
	UDC_TLX_EVT_ENUMDONE,
	UDC_TLX_EVT_SUSPEND,
	UDC_TLX_EVT_RESUME,
	UDC_TLX_EVT_SOF,
	UDC_TLX_EVT_SETUP,
	UDC_TLX_EVT_OUT,
	UDC_TLX_EVT_IN,
	UDC_TLX_EVT_XFER,
};

struct udc_tlx_evt {
	enum udc_tlx_evt_type type;
	uint8_t ep;
};

#ifndef CONFIG_UDC_TLX_MSG_QUEUE_SIZE
#define CONFIG_UDC_TLX_MSG_QUEUE_SIZE 32
#endif

#ifndef CONFIG_UDC_TLX_THREAD_STACK_SIZE
#define CONFIG_UDC_TLX_THREAD_STACK_SIZE 1024
#endif

K_MSGQ_DEFINE(drv_msgq, sizeof(struct udc_tlx_evt),
	      CONFIG_UDC_TLX_MSG_QUEUE_SIZE, sizeof(uint32_t));

static K_KERNEL_STACK_DEFINE(drv_stack, CONFIG_UDC_TLX_THREAD_STACK_SIZE);
static struct k_thread drv_stack_data;

/* Endpoint configurations registered with the UDC core. */
static struct udc_ep_config ep_cfg_out[UDC_TLX_NUM_BIDIR_EP];
static struct udc_ep_config ep_cfg_in[UDC_TLX_NUM_BIDIR_EP];

/*
 * Receive bounce buffer. The hardware requires the OUT DMA buffer length to
 * be a multiple of the endpoint MPS, therefore data is always received into
 * this buffer and then copied to the request net_buf.
 */
static uint8_t out_buf[UDC_TLX_NUM_BIDIR_EP][UDC_TLX_EP0_SIZE] __aligned(4);

/* EP0 control OUT state and receive buffer. */
enum udc_tlx_ep0_state {
	UDC_TLX_EP0_IDLE,
	UDC_TLX_EP0_SETUP,
	UDC_TLX_EP0_DATA_OUT,
	UDC_TLX_EP0_STATUS_OUT,
};

static enum udc_tlx_ep0_state ep0_state = UDC_TLX_EP0_IDLE;
static uint8_t ep0_buf[UDC_TLX_EP0_SIZE] __aligned(4);
static size_t ep0_remaining;

static void udc_tlx_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_tlx_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static void udc_tlx_fifo_init(void)
{
	usb0hw_set_grxfsiz(EPS_BUFFER_OUT_SIZE);
	eps_fifo.seg_addr = EPS_BUFFER_OUT_SIZE;
	eps_fifo.remaining_size = EPS_BUFFER_IN_SIZE;

	/* Allocate EP0 IN TX FIFO */
	usb0hw_set_epin_size(USB0_EP0, eps_fifo.seg_addr, UDC_TLX_EP0_SIZE);
	eps_fifo.seg_addr += UDC_TLX_EP0_SIZE;
	eps_fifo.remaining_size -= UDC_TLX_EP0_SIZE;
}

/* ------------------------------------------------------------------ */
/* EP0 helpers                                                         */
/* ------------------------------------------------------------------ */

static void udc_tlx_ep0_arm_setup(const struct device *dev)
{
	ep0_state = UDC_TLX_EP0_SETUP;
	usb0hw_read_ep_data(USB0_EP0, ep0_buf, UDC_TLX_EP0_SIZE);
}

static void udc_tlx_ep0_arm_data_out(const struct device *dev, size_t length)
{
	ep0_state = UDC_TLX_EP0_DATA_OUT;
	ep0_remaining = length;
	udc_ep_set_busy(dev, USB_CONTROL_EP_OUT, true);
	usb0hw_read_ep_data(USB0_EP0, ep0_buf, UDC_TLX_EP0_SIZE);
}

static void udc_tlx_ep0_arm_status_out(const struct device *dev)
{
	ep0_state = UDC_TLX_EP0_STATUS_OUT;
	udc_ep_set_busy(dev, USB_CONTROL_EP_OUT, true);
	usb0hw_read_ep_data(USB0_EP0, ep0_buf, UDC_TLX_EP0_SIZE);
}

/*
 * Allocate a control OUT buffer and arm EP0 OUT. A length of zero is used
 * for the status OUT stage.
 */
static int udc_tlx_ctrl_feed_dout(const struct device *dev, size_t length)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;
	size_t size = (length == 0) ? 0 : ROUND_UP(length, UDC_TLX_EP0_SIZE);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, size);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&cfg->fifo, buf);

	if (length == 0) {
		udc_tlx_ep0_arm_status_out(dev);
	} else {
		udc_tlx_ep0_arm_data_out(dev, length);
	}

	return 0;
}

/* ------------------------------------------------------------------ */
/* Control transfer handling                                           */
/* ------------------------------------------------------------------ */

static int udc_tlx_handler_setup(const struct device *dev)
{
	struct net_buf *buf;
	int err;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT,
			     sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		udc_tlx_ep0_arm_setup(dev);
		return -ENOMEM;
	}

	udc_ep_buf_set_setup(buf);
	memcpy(buf->data, ep0_buf, sizeof(struct usb_setup_packet));
	net_buf_add(buf, sizeof(struct usb_setup_packet));

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		LOG_DBG("feed for data OUT");
		err = udc_tlx_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static void udc_tlx_event_xfer_ctrl_in(const struct device *dev,
				       struct net_buf *const buf)
{
	if (udc_ctrl_stage_is_status_in(dev) ||
	    udc_ctrl_stage_is_no_data(dev)) {
		/* Status stage finished, notify upper layer */
		udc_ctrl_submit_status(dev, buf);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_status_out(dev)) {
		/* IN transfer finished, feed buffer for status OUT */
		net_buf_unref(buf);
		udc_tlx_ctrl_feed_dout(dev, 0);
	} else {
		/* Control transfer complete, arm EP0 for next SETUP */
		udc_tlx_ep0_arm_setup(dev);
	}
}

static void udc_tlx_handler_ep0_out(const struct device *dev)
{
	struct net_buf *buf;
	size_t copy_len;
	size_t xfered_len = usb0hw_get_epout_len(USB0_EP0);

	switch (ep0_state) {
	case UDC_TLX_EP0_DATA_OUT:
		buf = udc_buf_peek(dev, USB_CONTROL_EP_OUT);
		if (buf == NULL) {
			LOG_ERR("EP0 OUT data queue is empty");
			udc_tlx_ep0_arm_setup(dev);
			return;
		}

		copy_len = MIN(xfered_len, ep0_remaining);
		memcpy(net_buf_tail(buf), ep0_buf, copy_len);
		net_buf_add(buf, copy_len);
		ep0_remaining -= copy_len;

		if (ep0_remaining > 0) {
			/* More data expected */
			usb0hw_read_ep_data(USB0_EP0, ep0_buf,
					    UDC_TLX_EP0_SIZE);
			return;
		}

		/* Data stage finished */
		buf = udc_buf_get(dev, USB_CONTROL_EP_OUT);
		udc_ep_set_busy(dev, USB_CONTROL_EP_OUT, false);
		ep0_state = UDC_TLX_EP0_IDLE;
		udc_ctrl_update_stage(dev, buf);
		if (udc_ctrl_stage_is_status_in(dev)) {
			udc_ctrl_submit_s_out_status(dev, buf);
		}
		break;

	case UDC_TLX_EP0_STATUS_OUT:
		buf = udc_buf_get(dev, USB_CONTROL_EP_OUT);
		udc_ep_set_busy(dev, USB_CONTROL_EP_OUT, false);
		ep0_state = UDC_TLX_EP0_IDLE;
		udc_ctrl_update_stage(dev, buf);
		udc_ctrl_submit_status(dev, buf);
		udc_tlx_ep0_arm_setup(dev);
		break;

	default:
		LOG_WRN("Unexpected EP0 OUT event in state %d", ep0_state);
		udc_tlx_ep0_arm_setup(dev);
		break;
	}
}

/* ------------------------------------------------------------------ */
/* Transfer helpers                                                    */
/* ------------------------------------------------------------------ */

static void udc_tlx_xfer_in_next(const struct device *dev, uint8_t ep)
{
	struct net_buf *buf;
	uint8_t *data;

	if (udc_ep_is_busy(dev, ep)) {
		return;
	}

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		return;
	}

	data = (buf->len) ? buf->data : NULL;
	usb0hw_write_ep_data(USB_EP_GET_IDX(ep), data, buf->len);
	udc_ep_set_busy(dev, ep, true);
}

static void udc_tlx_xfer_out_next(const struct device *dev, uint8_t ep)
{
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, ep);
	uint16_t mps = udc_mps_ep_size(cfg);

	if (udc_ep_is_busy(dev, ep)) {
		return;
	}

	if (udc_buf_peek(dev, ep) == NULL) {
		return;
	}

	usb0hw_read_ep_data(USB_EP_GET_IDX(ep), out_buf[USB_EP_GET_IDX(ep)], mps);
	udc_ep_set_busy(dev, ep, true);
}

static void udc_tlx_handler_in(const struct device *dev, uint8_t ep)
{
	struct net_buf *buf;

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		LOG_ERR("ep 0x%02x queue is empty", ep);
		return;
	}

	if (ep != USB_CONTROL_EP_IN && udc_ep_buf_has_zlp(buf)) {
		/* Whole transfer is done, send ZLP */
		udc_ep_buf_clear_zlp(buf);
		usb0hw_write_ep_data(USB_EP_GET_IDX(ep), NULL, 0);
		return;
	}

	buf = udc_buf_get(dev, ep);
	udc_ep_set_busy(dev, ep, false);

	if (ep == USB_CONTROL_EP_IN) {
		udc_tlx_event_xfer_ctrl_in(dev, buf);
	} else {
		if (udc_submit_ep_event(dev, buf, 0)) {
			/* The completion event was dropped (usbd_msgq full).
			 * The buffer is no longer referenced anywhere, so
			 * release it explicitly. Leaking it here would
			 * permanently exhaust the class IN net_buf pool
			 * (e.g. HID CONFIG_USBD_HID_IN_BUF_COUNT=2) and all
			 * subsequent report submissions would fail with
			 * -ENOMEM until reboot. */
			LOG_ERR("ep 0x%02x: failed to submit ep event", ep);
			udc_ep_buf_free(dev, buf);
		}
		udc_tlx_xfer_in_next(dev, ep);
	}
}

static void udc_tlx_handler_out(const struct device *dev, uint8_t ep)
{
	struct udc_ep_config *cfg;
	struct net_buf *buf;
	uint8_t idx = USB_EP_GET_IDX(ep);
	uint16_t mps;
	size_t copy_len;
	size_t xfered_len;

	if (ep == USB_CONTROL_EP_OUT) {
		udc_tlx_handler_ep0_out(dev);
		return;
	}

	cfg = udc_get_ep_cfg(dev, ep);
	mps = udc_mps_ep_size(cfg);
	xfered_len = usb0hw_get_epout_len(idx);

	buf = udc_buf_peek(dev, ep);
	if (buf == NULL) {
		LOG_ERR("ep 0x%02x queue is empty", ep);
		return;
	}

	copy_len = MIN(xfered_len, net_buf_tailroom(buf));
	memcpy(net_buf_tail(buf), out_buf[idx], copy_len);
	net_buf_add(buf, copy_len);

	if (xfered_len == mps && net_buf_tailroom(buf) > 0) {
		/* More data expected */
		usb0hw_read_ep_data(idx, out_buf[idx], mps);
		return;
	}

	buf = udc_buf_get(dev, ep);
	udc_ep_set_busy(dev, ep, false);
	if (udc_submit_ep_event(dev, buf, 0)) {
		/* The completion event was dropped (usbd_msgq full).
		 * Release the buffer to avoid permanently leaking it
		 * from the class OUT net_buf pool. */
		LOG_ERR("ep 0x%02x: failed to submit ep event", ep);
		udc_ep_buf_free(dev, buf);
	}
	udc_tlx_xfer_out_next(dev, ep);
}

static void udc_tlx_handler_reset(const struct device *dev)
{
	usb0hw_reset();
	udc_set_suspended(dev, false);

#if IS_ENABLED(CONFIG_UDC_TELINK_TLX_HIGH_SPEED)
	/* Bus reset clears DEVSPD in DCFG register, must re-program for HS
	 * chirp negotiation to succeed (hardware resets this bit to FS).
	 */
	reg_usb_dcfg = (reg_usb_dcfg & (~FLD_USB_DCFG_DEVSPD)) |
		       MASK_VAL(FLD_USB_DCFG_DEVSPD, USB0_SPEED_HIGH, FLD_USB_DCFG_DESCDMA, 1);
	/* HS requires USBTRDTIM = 9 (turnaround time), FS needs 5 */
	reg_usb_gusbcfg = (reg_usb_gusbcfg & ~FLD_USB_GUSBCFG_USBTRDTIM) |
			  MASK_VAL(FLD_USB_GUSBCFG_USBTRDTIM, 9);
#else
	reg_usb_dcfg = (reg_usb_dcfg & (~FLD_USB_DCFG_DEVSPD)) |
		       MASK_VAL(FLD_USB_DCFG_DEVSPD, USB0_SPEED_FULL, FLD_USB_DCFG_DESCDMA, 1);
	reg_usb_gusbcfg = (reg_usb_gusbcfg & ~FLD_USB_GUSBCFG_USBTRDTIM) |
			  MASK_VAL(FLD_USB_GUSBCFG_USBTRDTIM, 5);
#endif

	/* Signal hardware that configuration is done and HS chirp can start.
	 * This MUST be called after programming DCFG for correct speed.
	 */
	usb0hw_set_pwronprgdone();

	udc_tlx_fifo_init();
	udc_tlx_ep0_arm_setup(dev);
}

static void udc_tlx_handler_enumdone(const struct device *dev)
{
	/* Speed negotiation is complete, ENUMSPD register is now valid.
	 * Report bus reset to the UDC core only AFTER this point so that
	 * the device_speed() callback returns the correct value.
	 */
	printk("USB enumeration done, speed: %s\n",
		usb0hw_get_speed() == USB0_SPEED_HIGH ? "HIGH" : "FULL");
	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

/* ------------------------------------------------------------------ */
/* Driver thread                                                       */
/* ------------------------------------------------------------------ */

static void udc_tlx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct udc_tlx_evt evt;

	while (true) {
		k_msgq_get(&drv_msgq, &evt, K_FOREVER);

		switch (evt.type) {
		case UDC_TLX_EVT_RESET:
			udc_tlx_handler_reset(dev);
			break;
		case UDC_TLX_EVT_ENUMDONE:
			udc_tlx_handler_enumdone(dev);
			break;
		case UDC_TLX_EVT_SUSPEND:
			udc_set_suspended(dev, true);
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
			break;
		case UDC_TLX_EVT_RESUME:
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
			break;
		case UDC_TLX_EVT_SOF:
			udc_submit_event(dev, UDC_EVT_SOF, 0);
			break;
		case UDC_TLX_EVT_SETUP:
			udc_tlx_handler_setup(dev);
			break;
		case UDC_TLX_EVT_OUT:
			udc_tlx_handler_out(dev, evt.ep);
			break;
		case UDC_TLX_EVT_IN:
			udc_tlx_handler_in(dev, evt.ep);
			break;
		case UDC_TLX_EVT_XFER:
			if (USB_EP_DIR_IS_IN(evt.ep)) {
				udc_tlx_xfer_in_next(dev, evt.ep);
			} else {
				udc_tlx_xfer_out_next(dev, evt.ep);
			}
			break;
		}
	}
}

/* ------------------------------------------------------------------ */
/* Interrupt handling                                                  */
/* ------------------------------------------------------------------ */

static void udc_tlx_irq_out(void)
{
	struct udc_tlx_evt evt;

	for (uint8_t ep_num = 0; ep_num < UDC_TLX_NUM_BIDIR_EP; ep_num++) {
		if (!((usb0hw_get_daint() >> 16) & BIT(ep_num))) {
			continue;
		}

		unsigned int doepint = usb0hw_get_doepint(ep_num);

		if (doepint & FLD_USB_DOEPINT_SETUP) {
			/* SETUP packet takes priority - clear both SETUP and XFERCOMPL
			 * to avoid duplicate events (SETUP reception also asserts XFERCOMPL)
			 */
			usb0hw_clear_doepint(ep_num, FLD_USB_DOEPINT_SETUP |
						   FLD_USB_DOEPINT_XFERCOMPL);
			evt.type = UDC_TLX_EVT_SETUP;
			evt.ep = USB_EP_GET_ADDR(ep_num, USB_EP_DIR_OUT);
			k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
		} else if (doepint & FLD_USB_DOEPINT_XFERCOMPL) {
			usb0hw_clear_doepint(ep_num, FLD_USB_DOEPINT_XFERCOMPL);
			evt.type = UDC_TLX_EVT_OUT;
			evt.ep = USB_EP_GET_ADDR(ep_num, USB_EP_DIR_OUT);
			k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
		}
		if (doepint & FLD_USB_DOEPINT_STSPHSERCVD) {
			usb0hw_clear_doepint(ep_num, FLD_USB_DOEPINT_STSPHSERCVD);
		}
	}
}

static void udc_tlx_irq_in(void)
{
	struct udc_tlx_evt evt;

	for (uint8_t ep_num = 0; ep_num < UDC_TLX_NUM_BIDIR_EP; ep_num++) {
		if (!(usb0hw_get_daint() & BIT(ep_num))) {
			continue;
		}

		unsigned int diepint = usb0hw_get_diepint(ep_num);

		if (diepint & FLD_USB_DIEPINT_XFERCOMPL) {
			usb0hw_clear_diepint(ep_num, FLD_USB_DIEPINT_XFERCOMPL);
			evt.type = UDC_TLX_EVT_IN;
			evt.ep = USB_EP_GET_ADDR(ep_num, USB_EP_DIR_IN);
			k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
		}
	}
}

void __attribute__((weak)) usb_sof_callback(void)
{
    /* Default empty implementation */
}

__attribute__((section(".ram_code"))) static void udc_tlx_isr(const void *arg)
{
	ARG_UNUSED(arg);

	unsigned int status = usb0hw_get_gintsts() & reg_usb_gintmsk;
	struct udc_tlx_evt evt;
	// printk("tlx isr %d\n", status);
	if (status & FLD_USB_GINTSTS_ENUMDONE) {
		usb0hw_clear_gintsts(FLD_USB_GINTSTS_ENUMDONE);
		evt.type = UDC_TLX_EVT_ENUMDONE;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	if (status & FLD_USB_GINTSTS_USBRST) {
		usb0hw_clear_gintsts(FLD_USB_GINTSTS_USBRST);
		/* Do NOT set PWRONPRGDONE here - it must be called AFTER DCFG
		 * is programmed for HS mode in the reset handler. Setting it
		 * too early causes chirp negotiation to start in FS mode.
		 */
		evt.type = UDC_TLX_EVT_RESET;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	if (status & FLD_USB_GINTSTS_USBSUSP) {
		usb0hw_clear_gintsts(FLD_USB_GINTSTS_USBSUSP);
		evt.type = UDC_TLX_EVT_SUSPEND;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	if (status & FLD_USB_GINTSTS_WKUPINT) {
		usb0hw_clear_gintsts(FLD_USB_GINTSTS_WKUPINT);
		evt.type = UDC_TLX_EVT_RESUME;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	if (status & FLD_USB_GINTSTS_SOF) {
		usb0hw_clear_gintsts(FLD_USB_GINTSTS_SOF);
		usb_sof_callback();
		/* Do NOT queue SOF events to the driver thread. In high-speed
		 * mode SOF fires every 125 us, which floods drv_msgq and can
		 * silently drop critical SETUP/XFERCOMPL events (K_NO_WAIT),
		 * breaking enumeration. The UDC core ignores SOF until the
		 * device is configured anyway.
		 */
	}

	if (status & FLD_USB_GINTSTS_RESETDET) {
		usb0hw_clear_gintsts(FLD_USB_GINTSTS_RESETDET);
	}

	if (status & FLD_USB_GINTSTS_OEPINT) {
		udc_tlx_irq_out();
	}

	if (status & FLD_USB_GINTSTS_IEPINT) {
		udc_tlx_irq_in();
	}
}

static int udc_tlx_irq_init(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), udc_tlx_isr, NULL, 0);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

/* ------------------------------------------------------------------ */
/* UDC API                                                             */
/* ------------------------------------------------------------------ */

static int udc_tlx_ep_enable(const struct device *dev,
			     struct udc_ep_config *const cfg)
{
	uint8_t idx = USB_EP_GET_IDX(cfg->addr);
	uint16_t mps = udc_mps_ep_size(cfg);
	usb0_dir_e dir = USB_EP_DIR_IS_IN(cfg->addr) ? USB0_DIR_IN : USB0_DIR_OUT;
	usb0_ep_type_e type;

	switch (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		type = USB0_EP_TYPE_CONTROL;
		break;
	case USB_EP_TYPE_ISO:
		type = USB0_EP_TYPE_ISOCHRONOUS;
		break;
	case USB_EP_TYPE_BULK:
		type = USB0_EP_TYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		type = USB0_EP_TYPE_INTERRUPT;
		break;
	default:
		return -EINVAL;
	}

	if (idx == 0) {
		/* EP0 is handled by usb0hw_reset()/usb0hw_fifo_init() */
		return 0;
	}

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		if (eps_fifo.remaining_size < mps) {
			LOG_ERR("Not enough FIFO for ep 0x%02x", cfg->addr);
			return -ENOMEM;
		}

		usb0hw_set_epin_size(idx, eps_fifo.seg_addr, mps);
		eps_fifo.seg_addr += mps;
		eps_fifo.remaining_size -= mps;
	}

	usb0hw_ep_open(idx, dir, type, mps);
	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	return 0;
}

static int udc_tlx_ep_disable(const struct device *dev,
			      struct udc_ep_config *const cfg)
{
	uint8_t idx = USB_EP_GET_IDX(cfg->addr);

	if (idx == 0) {
		return 0;
	}

	usb0hw_ep_close(idx,
			USB_EP_DIR_IS_IN(cfg->addr) ? USB0_DIR_IN : USB0_DIR_OUT);

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		usb0hw_flush_tx_fifo(idx);
	} else {
		usb0hw_flush_rx_fifo();
	}

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	return 0;
}

static int udc_tlx_ep_set_halt(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	uint8_t idx = USB_EP_GET_IDX(cfg->addr);

	LOG_DBG("Halt ep 0x%02x", cfg->addr);

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		usb0hw_set_inep_stall(idx);
	} else {
		usb0hw_set_outep_stall(idx);
	}

	if (idx == 0) {
		udc_tlx_ep0_arm_setup(dev);
	}

	return 0;
}

static int udc_tlx_ep_clear_halt(const struct device *dev,
				 struct udc_ep_config *const cfg)
{
	uint8_t idx = USB_EP_GET_IDX(cfg->addr);

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		usb0hw_clear_epin_stall(idx);
	} else {
		usb0hw_clear_epout_stall(idx);
	}

	return 0;
}

static int udc_tlx_ep_enqueue(const struct device *dev,
			      struct udc_ep_config *const cfg,
			      struct net_buf *const buf)
{
	struct udc_tlx_evt evt = {
		.type = UDC_TLX_EVT_XFER,
		.ep = cfg->addr,
	};

	udc_buf_put(cfg, buf);
	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);

	return 0;
}

static int udc_tlx_ep_dequeue(const struct device *dev,
			      struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	uint8_t idx = USB_EP_GET_IDX(cfg->addr);

	buf = udc_buf_get_all(dev, cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(dev, cfg->addr, false);

	if (idx != 0) {
		if (USB_EP_DIR_IS_IN(cfg->addr)) {
			usb0hw_flush_tx_fifo(idx);
		} else {
			usb0hw_flush_rx_fifo();
		}
	}

	return 0;
}

static int udc_tlx_set_address(const struct device *dev, const uint8_t addr)
{
	usb0hw_set_address(addr);
	return 0;
}

static int udc_tlx_host_wakeup(const struct device *dev)
{
    if (!udc_is_suspended(dev)) {
        return 0;
    }

    usb0hw_remote_wakeup();

    udc_set_suspended(dev, false);
    udc_submit_event(dev, UDC_EVT_RESUME, 0);

    return 0;
}

static enum udc_bus_speed udc_tlx_device_speed(const struct device *dev)
{
	usb0_speed_e speed = usb0hw_get_speed();

	/* USB spec: HS EP0 MPS is 64 bytes, FS EP0 MPS is also 64 bytes,
	 * but we must report actual speed for HS descriptor selection.
	 * ENUMSPD register is valid immediately after bus reset completes
	 * (USBRST interrupt), ENUMDONE is just an interrupt notification.
	 */
	return (speed == USB0_SPEED_HIGH) ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

static int udc_tlx_enable(const struct device *dev)
{
	usb0hw_reset();
	udc_tlx_fifo_init();
	printk("udc_tlx_enable\n");
	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				   USB_EP_TYPE_CONTROL, UDC_TLX_EP0_SIZE, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				   USB_EP_TYPE_CONTROL, UDC_TLX_EP0_SIZE, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	usb0hw_soft_connect();
	udc_tlx_ep0_arm_setup(dev);

	return 0;
}

static int udc_tlx_disable(const struct device *dev)
{
	usb0hw_soft_disconnect();

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	return 0;
}

static int udc_tlx_init(const struct device *dev)
{
	int ret;

#if IS_ENABLED(CONFIG_UDC_TELINK_TLX_HIGH_SPEED)
	usb0hw_init(USB0_SPEED_HIGH);
#else
	usb0hw_init(USB0_SPEED_FULL);
#endif
	printk("udc_tlx_init\n");
	ret = udc_tlx_irq_init();
	if (ret) {
		return ret;
	}

	/* Keep the controller hidden until udc_tlx_enable() */
	usb0hw_soft_disconnect();

	return 0;
}

static int udc_tlx_shutdown(const struct device *dev)
{
	usb0hw_power_down();
	return 0;
}

static int udc_tlx_driver_init(const struct device *dev)
{
	struct udc_data *data = dev->data;
	int err;

	k_mutex_init(&data->mutex);

	k_thread_create(&drv_stack_data, drv_stack,
			K_KERNEL_STACK_SIZEOF(drv_stack),
			udc_tlx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(8), 0, K_NO_WAIT);
	k_thread_name_set(&drv_stack_data, "udc_tl322x");

	for (int i = 0; i < UDC_TLX_NUM_BIDIR_EP; i++) {
		ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			ep_cfg_out[i].caps.control = 1;
			ep_cfg_out[i].caps.mps = UDC_TLX_EP0_SIZE;
		} else {
			ep_cfg_out[i].caps.bulk = 1;
			ep_cfg_out[i].caps.interrupt = 1;
			ep_cfg_out[i].caps.mps = UDC_TLX_EP0_SIZE;
		}

		ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < UDC_TLX_NUM_BIDIR_EP; i++) {
		ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			ep_cfg_in[i].caps.control = 1;
			ep_cfg_in[i].caps.mps = UDC_TLX_EP0_SIZE;
		} else {
			ep_cfg_in[i].caps.bulk = 1;
			ep_cfg_in[i].caps.interrupt = 1;
			ep_cfg_in[i].caps.mps = UDC_TLX_EP0_SIZE;
		}

		ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	data->caps.rwup = true;
	data->caps.addr_before_status = true;
	data->caps.mps0 = UDC_TLX_MPS0;
	data->caps.out_ack = false;
	data->caps.hs = IS_ENABLED(CONFIG_UDC_TELINK_TLX_HIGH_SPEED);
	data->caps.can_detect_vbus = false;

	return 0;
}

static const struct udc_api udc_tlx_api = {
	.lock = udc_tlx_lock,
	.unlock = udc_tlx_unlock,
	.init = udc_tlx_init,
	.enable = udc_tlx_enable,
	.disable = udc_tlx_disable,
	.shutdown = udc_tlx_shutdown,
	.set_address = udc_tlx_set_address,
	.host_wakeup = udc_tlx_host_wakeup,
	.device_speed = udc_tlx_device_speed,
	.ep_try_config = NULL,
	.ep_enable = udc_tlx_ep_enable,
	.ep_disable = udc_tlx_ep_disable,
	.ep_set_halt = udc_tlx_ep_set_halt,
	.ep_clear_halt = udc_tlx_ep_clear_halt,
	.ep_enqueue = udc_tlx_ep_enqueue,
	.ep_dequeue = udc_tlx_ep_dequeue,
};

static struct udc_data udc_tlx_data = {
	.mutex = Z_MUTEX_INITIALIZER(udc_tlx_data.mutex),
	.priv = NULL,
};

DEVICE_DT_INST_DEFINE(0, udc_tlx_driver_init, NULL,
		      &udc_tlx_data, NULL,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &udc_tlx_api);