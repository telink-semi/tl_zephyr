/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_SOC_RISCV_TELINK_TL322X
#include "driver.h"
#endif

#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/usb/usb_device.h>

#include <soc.h>

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_tl322x);

#define DT_DRV_COMPAT telink_tl322x_usbd

#define USBD_TLX_IRQN_BY_IDX(idx)         DT_INST_IRQ_BY_IDX(0, idx, irq)
#define USBD_TLX_IRQ_PRIORITY_BY_IDX(idx) DT_INST_IRQ_BY_IDX(0, idx, priority)

enum usbd_endpoint_index_e {
	USBD_EP0_IDX = 0, /* only for control transfer */
	USBD_EP1_IDX = 1, /* IN and OUT */
	USBD_EP2_IDX = 2, /* IN and OUT */
	USBD_EP3_IDX = 3, /* IN and OUT */
	USBD_EP4_IDX = 4, /* IN and OUT */
	USBD_EP5_IDX = 5, /* IN and OUT */
	USBD_EP6_IDX = 6, /* IN and OUT */
	USBD_EP7_IDX = 7, /* IN and OUT */
	USBD_EP8_IDX = 8, /* IN and OUT */
};

enum usbd_endpoint_index_e endpoint_idx[] = {USBD_EP0_IDX, USBD_EP1_IDX, USBD_EP2_IDX,
					     USBD_EP3_IDX, USBD_EP4_IDX, USBD_EP5_IDX,
					     USBD_EP6_IDX, USBD_EP7_IDX, USBD_EP8_IDX};

#define USBD_EPIN_BUSY_RETRY_TIMEOUT_US 10000

#define USBD_EP_TOTAL_CNT  (sizeof(endpoint_idx) / sizeof(enum usbd_endpoint_index_e))
#define USBD_EP_IN_OUT_CNT (USBD_EP_TOTAL_CNT - 1)

/** @brief The value of direction bit for the IN endpoint direction. */
#define USBD_EP_DIR_IN (1U << 7)

/** @brief The value of direction bit for the OUT endpoint direction. */
#define USBD_EP_DIR_OUT (0U << 7)

/**
 * @brief Macro for making the IN endpoint identifier from endpoint number.
 *
 * @details Macro that sets direction bit to make IN endpoint.
 *
 * @param[in] epn Endpoint number.
 *
 * @return IN Endpoint identifier.
 */
#define USBD_EPIN(epn) (((uint8_t)(epn)) | USBD_EP_DIR_IN)

/**
 * @brief Macro for making the OUT endpoint identifier from endpoint number.
 *
 * @details Macro that sets direction bit to make OUT endpoint.
 *
 * @param[in] epn Endpoint number.
 *
 * @return OUT Endpoint identifier.
 */
#define USBD_EPOUT(epn) (((uint8_t)(epn)) | USBD_EP_DIR_OUT)

#define EP_DATA_BUF_LEN (64)

/* The total hardware buffer size */
#define EPS_BUFFER_TOTAL_SIZE (8 * 1024)

#define EPS_BUFFER_OUT_SIZE (0x100)

#define EPS_BUFFER_IN_SIZE (EPS_BUFFER_TOTAL_SIZE - EPS_BUFFER_OUT_SIZE)

/**
 * @brief Endpoint buffer information.
 *
 * @param init_list			ep_idx that has been configured with BUF address
 * @param seg_addr			Available starting address of the USB endpoint cache.
 * @param init_num			Number of eps whose BUF address has been configured Max
 * packet size supported by endpoint.
 * @param remaining_size	The remaining available size of the USB endpoint cache.
 */
struct ep_buf_t {
	enum usbd_endpoint_index_e init_list[USBD_EP_TOTAL_CNT];
	uint8_t seg_addr;
	uint8_t init_num;
	uint16_t remaining_size;
};

static struct ep_buf_t eps_buf_inf = {.init_list = {0, 0, 0, 0, 0, 0, 0, 0, 0},
				      .seg_addr = 0,
				      .init_num = 0,
				      .remaining_size = EPS_BUFFER_IN_SIZE};

/**
 * @brief Endpoint configuration.
 *
 * @param cb		Endpoint callback.
 * @param max_sz	Max packet size supported by endpoint.
 * @param en		Enable/Disable flag.
 * @param addr		Endpoint address.
 * @param type		Endpoint transfer type.
 * @param stall		Endpoint stall flag.
 * @param out_ack	Endpoint ready for receive data flag.
 */
struct tlx_usbd_ep_cfg {
	usb_dc_ep_callback cb;
	unsigned short max_sz; // uint32_t -> unsigned short
	bool en;
	uint8_t addr;
	enum usb_dc_ep_transfer_type type;
	bool stall;
	bool out_ack;
};

/**
 * @brief Endpoint buffer
 *
 * @param total_len		Total length to be read/written.
 * @param left_len		Remaining length to be read/written.
 * @param data			Pointer to data buffer for the endpoint.
 * @param current_pos	Pointer to the current offset in the endpoint buffer.
 */
struct tlx_usbd_ep_buf {
	uint32_t total_len;
	uint32_t left_len;
	uint8_t *data;
	uint8_t *current_pos;
};

/**
 * @brief Endpoint context
 *
 * @param cfg	Endpoint configuration
 * @param buf	Endpoint buffer
 */
struct tlx_usbd_ep_ctx {
	struct tlx_usbd_ep_cfg cfg;
	struct tlx_usbd_ep_buf buf;
	bool reading;
	uint8_t writing_len;
	struct k_timer retry_timer;
};

/**
 * @brief USBD control structure
 *
 * @param status_cb			Status callback for USB DC notifications
 * @param attached			USBD Attached flag
 * @param ready				USBD Ready flag set after pullup
 * @param suspend			Suspend flag
 * @param wakeup_feature	Wakeup feature flag set by host
 * @param usb_work			USBD work item
 * @param drv_lock			Mutex for thread-safe tlx driver use
 * @param ep_ctx			Endpoint contexts
 */
struct tlx_usbd_ctx {
	usb_dc_status_callback status_cb;
	bool attached;
	bool ready;
	bool suspend;
	bool wakeup_feature;
	struct k_work usb_work;
	struct k_mutex drv_lock;
	struct tlx_usbd_ep_ctx ep_ctx[USBD_EP_TOTAL_CNT];
};

static struct tlx_usbd_ctx usbd_ctx = {
	.attached = false,
	.ready = false,
	.suspend = true,
	.wakeup_feature = false,
};

static inline struct tlx_usbd_ctx *get_usbd_ctx(void)
{
	return &usbd_ctx;
}

static inline bool dev_attached(void)
{
	return get_usbd_ctx()->attached;
}

static inline bool dev_ready(void)
{
	return get_usbd_ctx()->ready;
}

static inline bool ep_is_valid(const uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);

	if (ep_idx > USBD_EP_IN_OUT_CNT) {
		LOG_ERR("Endpoit index %d is out of range.", ep_idx);
		return false;
	}

	return true;
}

/** @brief Gets the structure pointer to the corresponding endpoint */
static struct tlx_usbd_ep_ctx *endpoint_ctx(const uint8_t ep)
{
	struct tlx_usbd_ctx *ctx;

	if (!ep_is_valid(ep)) {
		return NULL;
	}

	ctx = get_usbd_ctx();

	return &ctx->ep_ctx[USB_EP_GET_IDX(ep)];
}

/** @brief FIFO used for queuing up events from ISR. */
K_FIFO_DEFINE(usbd_evt_fifo);

/** @brief Work queue used for handling the ISR events (i.e. for notifying the USB
 * device stack, for executing the endpoints callbacks, etc.) out of the ISR context.
 *
 * @details The system work queue cannot be used for this purpose as it might be used
 * in applications for scheduling USB transfers and this could lead to a deadlock
 * when the USB device stack would not be notified about certain event because of
 * a system work queue item waiting for a USB transfer to be finished.
 */
static struct k_work_q usbd_work_queue;
#if CONFIG_USB_TELINK_TLX
/* USB_TLX_WORK_QUEUE_STACK_SIZE was 1024 by default but has been deprecated now.
 * It should be sized to accommodate the longest expected call stack of the USB device
 */
static K_KERNEL_STACK_DEFINE(usbd_work_queue_stack, 1024);
#endif

static inline void usbd_work_schedule(void)
{
	k_work_submit_to_queue(&usbd_work_queue, &(get_usbd_ctx()->usb_work));
}

enum usbd_event_type {
	USBD_EVT_OUT_COMPLETE,
	USBD_EVT_OUT_SETUP,
	USBD_EVT_OUT_RCVD,
	USBD_EVT_IN_COMPLETE,
	USBD_EVT_EP_WRITE_COMPLETE,
	USBD_EVT_EP_RETRY,
	USBD_EVT_RESET,
	USBD_EVT_SUSPEND,
	USBD_EVT_WAKEUP,
	USBD_EVT_REINIT,
};

struct usbd_mem_block {
	void *data;
};

struct usbd_event {
	sys_snode_t node;
	struct usbd_mem_block block;
	enum usbd_event_type evt_type;
	uint8_t ep_addr;
};

#define FIFO_ELEM_SZ    sizeof(struct usbd_event)
#define FIFO_ELEM_ALIGN sizeof(uint32_t)

#if CONFIG_USB_TELINK_TLX
K_MEM_SLAB_DEFINE(fifo_elem_slab, FIFO_ELEM_SZ, CONFIG_USB_TLX_EVT_QUEUE_SIZE, FIFO_ELEM_ALIGN);
#endif

/**
 * @brief Free previously allocated USBD event.
 *
 * @note Should be called after usbd_evt_get().
 *
 * @param ev	Pointer to the USBD event structure.
 */
static inline void usbd_evt_free(struct usbd_event *ev)
{
	k_mem_slab_free(&fifo_elem_slab, (void **)&ev->block.data);
}

/**
 * @brief Enqueue USBD event.
 *
 * @param ev	Pointer to the previously allocated and filled event structure.
 */
static inline void usbd_evt_put(struct usbd_event *ev)
{
	k_fifo_put(&usbd_evt_fifo, ev);
}

/**
 * @brief Get next enqueued USBD event if present.
 */
static inline struct usbd_event *usbd_evt_get(void)
{
	return k_fifo_get(&usbd_evt_fifo, K_NO_WAIT);
}

/**
 * @brief Drop all enqueued events.
 */
static inline void usbd_evt_flush(void)
{
	struct usbd_event *ev;

	do {
		ev = usbd_evt_get();
		if (ev) {
			usbd_evt_free(ev);
		}
	} while (ev != NULL);
}

static inline struct usbd_event *usbd_evt_alloc(void)
{
	struct usbd_event *ev;
	struct usbd_mem_block block;

	if (k_mem_slab_alloc(&fifo_elem_slab, (void **)&block.data, K_NO_WAIT)) {
		LOG_ERR("USBD event allocation failed!");

		/*
		 * Allocation may fail if workqueue thread is starved or event
		 * queue size is too small (CONFIG_USB_TLX_EVT_QUEUE_SIZE).
		 * Wipe all events, free the space and schedule
		 * reinitialization.
		 */
		usbd_evt_flush();

		if (k_mem_slab_alloc(&fifo_elem_slab, (void **)&block.data, K_NO_WAIT)) {
			LOG_ERR("USBD event memory corrupted");
			__ASSERT_NO_MSG(0);
			return NULL;
		}

		ev = (struct usbd_event *)block.data;
		ev->block = block;
		ev->evt_type = USBD_EVT_REINIT;
		usbd_evt_put(ev);
		usbd_work_schedule();

		return NULL;
	}

	ev = (struct usbd_event *)block.data;
	ev->block = block;

	return ev;
}

static void submit_usbd_event(enum usbd_event_type evt_type, uint8_t value)
{
	struct usbd_event *ev = usbd_evt_alloc();

	if (!ev) {
		return;
	}

	ev->evt_type = evt_type;

	if (ev->evt_type == USBD_EVT_IN_COMPLETE) {
		ev->ep_addr = value;
	} else if (ev->evt_type == USBD_EVT_EP_WRITE_COMPLETE) {
		ev->ep_addr = value;
	} else if (ev->evt_type == USBD_EVT_EP_RETRY) {
		ev->ep_addr = value;
	} else if (ev->evt_type == USBD_EVT_OUT_COMPLETE) {
		ev->ep_addr = value;
	} else if (ev->evt_type == USBD_EVT_OUT_SETUP) {
		ev->ep_addr = value;
	} else if (ev->evt_type == USBD_EVT_OUT_RCVD) {
		ev->ep_addr = value;
	}
	usbd_evt_put(ev);

	if (usbd_ctx.attached) {
		usbd_work_schedule();
	}
}

/**
 * @brief Reset endpoint state.
 *
 * @details Reset the internal logic state for a given endpoint.
 *
 * @param[in]  ep_idx   Endpoint number
 */
static void ep_ctx_reset(enum usbd_endpoint_index_e ep_idx)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	ep_ctx = endpoint_ctx(ep_idx);

	ep_ctx->buf.current_pos = ep_ctx->buf.data;
	ep_ctx->buf.total_len = 0;
	ep_ctx->buf.left_len = 0;
	ep_ctx->reading = false;
	ep_ctx->writing_len = 0;
}

static void ep_buf_clear(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);

	ep_ctx->buf.current_pos = ep_ctx->buf.data;
	ep_ctx->buf.total_len = 0;
	ep_ctx->buf.left_len = 0;
}

static void ep_buf_init(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);

	ep_ctx->buf.data = NULL;
	ep_buf_clear(ep);
}

static uint32_t ep_write(uint8_t ep, const uint8_t *data, uint32_t data_len)
{
	uint16_t i;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	struct tlx_usbd_ctx *ctx = get_usbd_ctx();
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);
	uint32_t valid_len = 0;

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);

	if (data_len > ep_ctx->cfg.max_sz) {
		valid_len = ep_ctx->cfg.max_sz;
	} else {
		valid_len = data_len;
	}

	ep_ctx->writing_len = valid_len;
	usb0hw_write_ep_data(ep_idx, data, valid_len);
	submit_usbd_event(USBD_EVT_EP_WRITE_COMPLETE, ep);

	k_mutex_unlock(&ctx->drv_lock);
	return valid_len;
}

static inline void usb_event_out_complete_handler(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);
	unsigned short xfered_len = usb0hw_get_epout_len(USB_EP_GET_IDX(ep));

	ep_ctx->cfg.out_ack = false;

	if ((USB_EP_GET_IDX(ep) == USBD_EP0_IDX)) {
		if ((xfered_len == 0)) {
			usb0hw_read_ep_data(USB_EP_GET_IDX(ep), ep_ctx->buf.data,
					    ep_ctx->cfg.max_sz);
			ep_ctx->cfg.out_ack = true;
		} else {
			ep_ctx->buf.left_len = ep_ctx->buf.total_len = xfered_len;
		}
	}
}

static inline void usb_event_out_setup_handler(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);
	unsigned short xfered_len = usb0hw_get_epout_len(USB_EP_GET_IDX(ep));

	if (get_usbd_ctx()->suspend) {
		get_usbd_ctx()->suspend = false;
		if (get_usbd_ctx()->status_cb) {
			get_usbd_ctx()->status_cb(USB_DC_RESUME, NULL);
		}
	}
	if (xfered_len > 0) {
		if ((get_usbd_ctx()->ep_ctx[USBD_EP0_IDX].buf.data[1] == USB_SREQ_SET_FEATURE) &&
		    (get_usbd_ctx()->ep_ctx[USBD_EP0_IDX].buf.data[2] == USB_SFS_REMOTE_WAKEUP)) {
			usbd_ctx.wakeup_feature = true;
		}
		if ((get_usbd_ctx()->ep_ctx[USBD_EP0_IDX].buf.data[1] == USB_SREQ_CLEAR_FEATURE) &&
		    (get_usbd_ctx()->ep_ctx[USBD_EP0_IDX].buf.data[2] == USB_SFS_REMOTE_WAKEUP)) {
			usbd_ctx.wakeup_feature = false;
		}

		ep_ctx->cfg.cb(ep, USB_DC_EP_SETUP);
	}
}

static inline void usb_event_out_rcvd_handler(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);
	unsigned short xfered_len = usb0hw_get_epout_len(USB_EP_GET_IDX(ep));

	ep_ctx->cfg.cb(ep, USB_DC_EP_DATA_OUT);
}

static inline void usb_event_in_handler(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);
	unsigned short xfered_len = usb0hw_get_epin_len(USB_EP_GET_IDX(ep));

	ep_ctx->cfg.cb(ep, USB_DC_EP_DATA_IN);

	if ((USB_EP_GET_IDX(ep) == USBD_EP0_IDX) || (USB_EP_GET_DIR(ep) == USB_EP_DIR_OUT)) {
		ep_ctx->cfg.out_ack = false;
	}
}

static inline void usb_event_ep_write_complete_handler(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);

	if (ep_ctx->cfg.cb) {
		ep_ctx->cfg.cb(ep, USB_DC_EP_DATA_IN);
	}
}

static inline void usb_event_ep_retry_handler(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx = endpoint_ctx(ep);

	if (ep_ctx->cfg.cb) {
		ep_ctx->cfg.cb(ep, USB_DC_EP_DATA_IN);
	}
}

static void usb_event_reset_handler(void)
{
	uint32_t i;

	usb0hw_reset();
	usb0hw_read_ep_data(USBD_EP0_IDX, endpoint_ctx(USBD_EP0_IDX)->buf.data,
			    endpoint_ctx(USBD_EP0_IDX)->cfg.max_sz);

	if (get_usbd_ctx()->suspend) {
		if (get_usbd_ctx()->status_cb) {
			get_usbd_ctx()->status_cb(USB_DC_CONNECTED, NULL);
		}
	}
	if (get_usbd_ctx()->status_cb) {
		LOG_DBG("USB reset");
		get_usbd_ctx()->status_cb(USB_DC_RESET, NULL);
	}
	if (get_usbd_ctx()->suspend) {
		get_usbd_ctx()->suspend = false;
		if (get_usbd_ctx()->status_cb) {
			LOG_DBG("USB resume");
			get_usbd_ctx()->status_cb(USB_DC_RESUME, NULL);
		}
	}
}

static inline void usb_event_suspend_handler(void)
{
	if (dev_ready()) {
		get_usbd_ctx()->suspend = true;
		if (get_usbd_ctx()->status_cb) {
			LOG_DBG("USB suspend");
			get_usbd_ctx()->status_cb(USB_DC_SUSPEND, NULL);

			if (!usbd_ctx.wakeup_feature) {
				LOG_DBG("USB disconnected");
				get_usbd_ctx()->status_cb(USB_DC_DISCONNECTED, NULL);
			}
		}
	}
}

static inline void usb_event_wakeup_handler(void)
{
	if (get_usbd_ctx()->suspend) {
		get_usbd_ctx()->suspend = false;
		if (get_usbd_ctx()->status_cb) {
			get_usbd_ctx()->status_cb(USB_DC_RESUME, NULL);
		}
	}
}

static inline void usb_irq_out(void)
{
	for (unsigned char ep_num = 0; ep_num < USBD_EP_TOTAL_CNT; ep_num++) {
		if ((usb0hw_get_daint() >> 16) & BIT(ep_num)) {
			unsigned int doepint = usb0hw_get_doepint(ep_num);

			if (doepint & FLD_USB_DOEPINT_XFERCOMPL) {
				usb0hw_clear_doepint(ep_num, FLD_USB_DOEPINT_XFERCOMPL);
				unsigned int len = usb0hw_get_epout_len(ep_num);
				submit_usbd_event(USBD_EVT_OUT_COMPLETE,
						  USB_EP_GET_ADDR(ep_num, USB_EP_DIR_OUT));
			}
			if (doepint & FLD_USB_DOEPINT_SETUP) {
				usb0hw_clear_doepint(ep_num, FLD_USB_DOEPINT_SETUP);
				submit_usbd_event(USBD_EVT_OUT_SETUP,
						  USB_EP_GET_ADDR(ep_num, USB_EP_DIR_OUT));
			}
			if (doepint & FLD_USB_DOEPINT_STSPHSERCVD) {
				usb0hw_clear_doepint(ep_num, FLD_USB_DOEPINT_STSPHSERCVD);
				submit_usbd_event(USBD_EVT_OUT_RCVD,
						  USB_EP_GET_ADDR(ep_num, USB_EP_DIR_OUT));
			}
		}
	}
}

static inline void usb_irq_in(void)
{
	for (unsigned char ep_num = 0; ep_num < USBD_EP_TOTAL_CNT; ep_num++) {
		if ((usb0hw_get_daint())&BIT(ep_num)) {
			unsigned int diepint = usb0hw_get_diepint(ep_num);
			if (diepint & FLD_USB_DIEPINT_XFERCOMPL) {
				usb0hw_clear_diepint(ep_num, FLD_USB_DIEPINT_XFERCOMPL);
				submit_usbd_event(USBD_EVT_IN_COMPLETE,
						  USB_EP_GET_ADDR(ep_num, USB_EP_DIR_IN));
			}
		}
	}
}

static inline void usb_irq_suspend(void)
{
	submit_usbd_event(USBD_EVT_SUSPEND, 0);
	usb0hw_clear_gintsts(FLD_USB_GINTSTS_USBSUSP);
}

static inline void usb_irq_wakeup(void)
{
	submit_usbd_event(USBD_EVT_WAKEUP, 0);
	usb0hw_clear_gintsts(FLD_USB_GINTSTS_WKUPINT);
}

static inline void usb_irq_reset(void)
{
	submit_usbd_event(USBD_EVT_RESET, 0);
	usb0hw_clear_gintsts(FLD_USB_GINTSTS_USBRST);
}

static inline void usb_irq_enumdone(void)
{
	usb0hw_clear_gintsts(FLD_USB_GINTSTS_ENUMDONE);
}

static inline void usb_irq_sof(void)
{
	usb0hw_clear_gintsts(FLD_USB_GINTSTS_SOF);
}

__attribute__((section(".ram_code"))) static void usb_irq_handler(void)
{
	unsigned int status = usb0hw_get_gintsts() & reg_usb_gintmsk;

	if (status != 0x8) {
		LOG_DBG("usb_irq(0x%X)", status);
	}
	if (status & FLD_USB_GINTSTS_ENUMDONE) {
		usb_irq_enumdone();
	}

	if (status & FLD_USB_GINTSTS_OEPINT) {
		usb_irq_out();
	}

	if (status & FLD_USB_GINTSTS_IEPINT) {
		usb_irq_in();
	}

	if (status & FLD_USB_GINTSTS_USBSUSP) {
		usb_irq_suspend();
	}

	if (status & FLD_USB_GINTSTS_WKUPINT) {
		usb_irq_wakeup();
	}

	if (status & FLD_USB_GINTSTS_USBRST) {
		usb_irq_reset();
	}

	if (status & FLD_USB_GINTSTS_SOF) {
		usb_irq_sof();
	}
}

static int usb_irq_init(void)
{
	IRQ_CONNECT(USBD_TLX_IRQN_BY_IDX(0), USBD_TLX_IRQ_PRIORITY_BY_IDX(0), usb_irq_handler, 0,
		    0);
	if (USBD_TLX_IRQN_BY_IDX(0) < CONFIG_2ND_LVL_ISR_TBL_OFFSET) {
		return -EINVAL;
	}
	plic_interrupt_enable(USBD_TLX_IRQN_BY_IDX(0) - CONFIG_2ND_LVL_ISR_TBL_OFFSET);
	plic_set_priority(USBD_TLX_IRQN_BY_IDX(0) - CONFIG_2ND_LVL_ISR_TBL_OFFSET,
			  USBD_TLX_IRQ_PRIORITY_BY_IDX(0));

	return 0;
}

void usbd_ep_stall(const unsigned char ep)
{
	unsigned char const ep_dir = USB_EP_GET_DIR(ep);
	unsigned char const ep_num = USB_EP_GET_IDX(ep);

	if (ep_dir == USB_EP_DIR_IN) {
		usb0hw_set_inep_stall(ep_num);
	} else {
		usb0hw_set_outep_stall(ep_num);
	}

	if (ep_num == 0) {
		/* receive next setup. */
		usb0hw_read_ep_data(USBD_EP0_IDX, endpoint_ctx(USBD_EP0_IDX)->buf.data,
				    endpoint_ctx(USBD_EP0_IDX)->cfg.max_sz);
	}
}

void usbd_ep_clear_stall(const unsigned char ep)
{
	unsigned char const ep_dir = USB_EP_GET_DIR(ep);
	unsigned char const ep_num = USB_EP_GET_IDX(ep);

	if (ep_dir == USB_EP_DIR_IN) {
		usb0hw_clear_epin_stall(ep_num);
	} else {
		usb0hw_clear_epout_stall(ep_num);
	}
}

/**
 * @brief Attach USB for device connection
 *
 * @details Function to attach USB for device connection. Upon success, the USB PLL
 * is enabled, and the USB device is now capable of transmitting and receiving on
 * the USB bus and of generating interrupts.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_attach(void)
{
	struct tlx_usbd_ctx *ctx = get_usbd_ctx();
	uint32_t i;

	if (ctx->attached) {
		return 0;
	}

	k_mutex_init(&ctx->drv_lock);

	for (uint32_t i = USBD_EP0_IDX; i < USBD_EP_TOTAL_CNT; i++) {
		ep_ctx_reset(i);
	}

	usb0hw_reset();

	ctx->attached = true;
	ctx->ready = true;

	return 0;
}

/**
 * @brief Detach the USB device
 *
 * @details Function to detach the USB device. Upon success, the USB hardware PLL
 * is powered down and USB communication is disabled.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_detach(void)
{
	struct tlx_usbd_ctx *ctx = get_usbd_ctx();
	struct tlx_usbd_ep_ctx *ep_ctx;
	uint8_t i;

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);

	for (i = USBD_EP1_IDX; i <= USBD_EP_IN_OUT_CNT; i++) {
		ep_ctx = endpoint_ctx(i);
		memset(ep_ctx, 0, sizeof(*ep_ctx));
	}
	ctx->attached = false;
	k_mutex_unlock(&ctx->drv_lock);

	return 0;
}

/**
 * @brief Reset the USB device
 *
 * @details This function returns the USB device and firmware back to it's initial state.
 * N.B. the USB PLL is handled by the usb_detach function
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_reset(void)
{
	int ret;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	LOG_DBG("USBD Reset");

	ret = usb_dc_detach();
	if (ret) {
		return ret;
	}

	ret = usb_dc_attach();
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * @brief Set USB device address
 *
 * @param[in] addr Device address
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("USBD Set_address");
	usb0hw_set_address(addr);
	return 0;
}

/**
 * @brief Set USB device controller status callback
 *
 * @details Function to set USB device controller status callback. The registered
 * callback is used to report changes in the status of the device controller. The
 * status code are described by the usb_dc_status_code enumeration.
 *
 * @param[in] cb Callback function
 */
void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	get_usbd_ctx()->status_cb = cb;
	LOG_DBG("status cb(0x%X)", cb);
}

/**
 * @brief check endpoint capabilities
 *
 * @details Function to check capabilities of an endpoint. usb_dc_ep_cfg_data structure
 * provides the endpoint configuration parameters: endpoint address, endpoint maximum
 * packet size and endpoint type. The driver should check endpoint capabilities and
 * return 0 if the endpoint configuration is possible.
 *
 * @param[in] cfg Endpoint config
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->ep_addr);

	LOG_DBG("ep 0x%02x, mps %d, type %d", ep_cfg->ep_addr, ep_cfg->ep_mps, ep_cfg->ep_type);

	if (ep_idx > USBD_EP8_IDX) {
		LOG_ERR("Endpoint index %d is out of range.", ep_idx);
		return -EINVAL;
	}

	if (ep_idx == USBD_EP0_IDX) {
		if (ep_cfg->ep_type != USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d can only be a control endpoint.", USBD_EP0_IDX);
			return -EINVAL;
		}
		if (ep_cfg->ep_mps > 64) {
			LOG_ERR("EP%d's max packet size is up to 64.", USBD_EP0_IDX);
			return -EINVAL;
		}
	} else if (USB_EP_DIR_IS_IN(ep_cfg->ep_addr)) {
		if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d cannot be a control endpoint.", ep_idx);
			return -EINVAL;
		}
	} else {
		if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d cannot be a control endpoint.", ep_idx);
			return -EINVAL;
		}

		if (ep_cfg->ep_mps > EPS_BUFFER_OUT_SIZE) {
			LOG_ERR("invalid endpoint max packet size: %d", ep_cfg->ep_mps);
			return -EINVAL;
		}
	}

	if (ep_cfg->ep_mps > EPS_BUFFER_IN_SIZE) {
		LOG_ERR("invalid endpoint max packet size: %d", ep_cfg->ep_mps);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Configure endpoint
 *
 * Function to configure an endpoint. usb_dc_ep_cfg_data structure provides
 * the endpoint configuration parameters: endpoint address, endpoint maximum
 * packet size and endpoint type.
 *
 * @param[in] cfg Endpoint config
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	struct tlx_usbd_ep_ctx *ep_ctx;
	uint8_t i;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->ep_addr);

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep_cfg->ep_addr);
	if (!ep_ctx) {
		return -EINVAL;
	}

	LOG_DBG("ep 0x%02x, ep_type:%d, ep_mps:%d", ep_cfg->ep_addr, ep_cfg->ep_type,
		ep_cfg->ep_mps);

	if (ep_idx == USBD_EP0_IDX) {
		if (ep_cfg->ep_type != USB_DC_EP_CONTROL) {
			LOG_ERR("EP%d only supports the control transmission mode.", USBD_EP0_IDX);
			return -EINVAL;
		}

		for (i = 0; i < eps_buf_inf.init_num; i++) {
			if (eps_buf_inf.init_list[i] == ep_idx) {
				LOG_DBG("ep%d buf address already configured", ep_idx);
				return 0;
			}
		}

		ep_ctx->cfg.max_sz = ep_cfg->ep_mps;
		usb0hw_set_epin_size(ep_idx, eps_buf_inf.seg_addr, ep_ctx->cfg.max_sz);
		eps_buf_inf.seg_addr += ep_ctx->cfg.max_sz;
		eps_buf_inf.remaining_size -= ep_ctx->cfg.max_sz;
	} else {
		if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
			LOG_ERR("Only EP%d supports the control transmission mode!", USBD_EP0_IDX);
			return -EINVAL;
		}

		for (i = 0; i < eps_buf_inf.init_num; i++) {
			if (eps_buf_inf.init_list[i] == ep_idx) {
				LOG_DBG("ep%d buf address already configured", ep_idx);
				return 0;
			}
		}

		if (eps_buf_inf.remaining_size < ep_cfg->ep_mps) {
			LOG_ERR("There is only %d bytes left for endpoint buffer.",
				eps_buf_inf.remaining_size);
			return -EINVAL;
		}

		ep_ctx->cfg.max_sz = ep_cfg->ep_mps;
		if (USB_EP_DIR_IS_IN(ep_ctx->cfg.addr)) {
			usb0hw_set_epin_size(ep_idx, eps_buf_inf.seg_addr, ep_ctx->cfg.max_sz);
			eps_buf_inf.seg_addr += ep_ctx->cfg.max_sz;
			eps_buf_inf.remaining_size -= ep_ctx->cfg.max_sz;
		}
	}

	ep_buf_init(ep_cfg->ep_addr);

	ep_ctx->cfg.addr = ep_cfg->ep_addr;
	ep_ctx->cfg.type = ep_cfg->ep_type;
	eps_buf_inf.init_list[eps_buf_inf.init_num] = ep_idx;
	eps_buf_inf.init_num++;

	return 0;
}

/**
 * @brief Set stall condition for the selected endpoint
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_set_stall(const uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	ep_ctx->cfg.stall = true;
	ep_buf_clear(ep);
	usbd_ep_stall(ep);
	LOG_DBG("Stall on ep%d", USB_EP_GET_IDX(ep));

	return 0;
}

/**
 * @brief Clear stall condition for the selected endpoint
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_clear_stall(const uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	ep_ctx->cfg.stall = false;
	usbd_ep_clear_stall(ep);
	LOG_DBG("Unstall on EP 0x%02x", ep);

	return 0;
}

/**
 * @brief Check if the selected endpoint is stalled
 *
 * @param[in]  ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 * @param[out] stalled	Endpoint stall status
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = ep_ctx->cfg.stall;

	return 0;
}

/**
 * @brief Halt the selected endpoint
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

/**
 * @brief Enable the selected endpoint
 *
 * @details Function to enable the selected endpoint. Upon success interrupts are
 * enabled for the corresponding endpoint and the endpoint is ready for
 * transmitting/receiving data.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_enable(const uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	LOG_DBG("EP enable: 0x%02x", ep);
	ep_ctx->cfg.en = true;

	if (dev_ready()) {
		ep_ctx->cfg.stall = false;

		if (USB_EP_GET_DIR(ep) == USB_EP_DIR_IN) {
			ep_ctx->buf.data = (uint8_t *)malloc(ep_ctx->cfg.max_sz);
			if (ep_ctx->buf.data == NULL) {
				LOG_ERR("ep(0X%x) malloc fail", ep);
				return -ENOSPC;
			}
		}

		if (USB_EP_GET_IDX(ep) != 0) {
			usb0hw_ep_open(USB_EP_GET_IDX(ep_ctx->cfg.addr),
				       USB_EP_GET_DIR(ep_ctx->cfg.addr) ? USB0_DIR_IN
									: USB0_DIR_OUT,
				       ep_ctx->cfg.type, ep_ctx->cfg.max_sz);
			return 0;
		}
	}

	return 0;
}

/**
 * @brief Disable the selected endpoint
 *
 * @details Function to disable the selected endpoint. Upon success interrupts are
 * disabled for the corresponding endpoint and the endpoint is no longer able for
 * transmitting/receiving data.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_disable(const uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		return -EALREADY;
	}

	if (ep_ctx->buf.data != NULL) {
		free(ep_ctx->buf.data);
		ep_ctx->buf.data = NULL;
	}

	LOG_DBG("EP disable: 0x%02x", ep);
	usb0hw_ep_close(USB_EP_GET_IDX(ep),
			USB_EP_GET_DIR(ep_ctx->cfg.addr) ? USB0_DIR_IN : USB0_DIR_OUT);
	ep_ctx_reset(USB_EP_GET_IDX(ep));

	if (USB_EP_GET_DIR(ep) == USB_EP_DIR_IN) {
		usb0hw_flush_tx_fifo(USB_EP_GET_IDX(ep));
	} else {
		usb0hw_flush_rx_fifo();
	}

	ep_ctx->cfg.stall = true;
	ep_ctx->cfg.en = false;

	return 0;
}

/**
 * @brief Flush the selected endpoint
 *
 * @details This function flushes the FIFOs for the selected endpoint.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_flush(const uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	ep_buf_clear(ep);
	LOG_DBG("ep%d flush", USB_EP_GET_IDX(ep));

	return 0;
}

/**
 * @brief Write data to the specified endpoint
 *
 * @details This function is called to write data to the specified endpoint. The
 * supplied usb_ep_callback function will be called when data is transmitted out.
 *
 * @param[in]  ep			Endpoint address corresponding to the one
 *							listed in the device configuration table
 * @param[in]  data			Pointer to data to write
 * @param[in]  data_len		Length of the data requested to write. This may
 *							be zero for a zero length status packet.
 * @param[out] ret_bytes	Bytes scheduled for transmission. This value
 *							may be NULL if the application expects all
 *							bytes to be written
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data, const uint32_t data_len,
		    uint32_t *const ret_bytes)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	LOG_DBG("ep 0x%02x, len %d", ep, data_len);

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}
	if (USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Endpoint 0x%02x is invalid, it has direaction error.", ep);
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}
	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	ep_ctx->cfg.stall = false;

	*ret_bytes = ep_write(ep, data, data_len);

	return 0;
}

/**
 * @brief Read data from the specified endpoint
 *
 * @details This function is called by the endpoint handler function, after an OUT
 * interrupt has been received for that EP. The application must only call this
 * function through the supplied usb_ep_callback function. This function clears
 * the ENDPOINT NAK, if all data in the endpoint FIFO has been read, so as to
 * accept more data from host.
 *
 * @param[in]  ep			Endpoint address corresponding to the one
 *							listed in the device configuration table
 * @param[in]  data			Pointer to data buffer to write to
 * @param[in]  max_data_len	Max length of data to read
 * @param[out] read_bytes	Number of bytes read. If data is NULL and
 *							max_data_len is 0 the number of bytes
 *							available for read should be returned.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read(const uint8_t ep, uint8_t *const data, const uint32_t max_data_len,
		   uint32_t *const read_bytes)
{
	int ret;

	LOG_DBG("dc_ep_read: ep 0x%02x, maxlen %d", ep, max_data_len);
	ret = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);

	if (ret) {
		return ret;
	}

	if (!data && !max_data_len) {
		return ret;
	}

	ret = usb_dc_ep_read_continue(ep);

	return ret;
}

/**
 * @brief Set callback function for the specified endpoint
 *
 * @details Function to set callback function for notification of data received and
 * available to application or transmit done on the selected endpoint, NULL if
 * callback not required by application code. The callback status code is
 * described by usb_dc_ep_cb_status_code.
 *
 * @param[in] ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 * @param[in] cb	Callback function
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	ep_ctx->cfg.cb = cb;

	return 0;
}

/**
 * @brief Read data from the specified endpoint
 *
 * @details This is similar to usb_dc_ep_read, the difference being that, it doesn't
 * clear the endpoint NAKs so that the consumer is not bogged down by further
 * upcalls till he is done with the processing of the data. The caller should
 * reactivate ep by invoking usb_dc_ep_read_continue() do so.
 *
 * @param[in]  ep			Endpoint address corresponding to the one
 *							listed in the device configuration table
 * @param[in]  data			Pointer to data buffer to write to
 * @param[in]  max_data_len Max length of data to read
 * @param[out] read_bytes	Number of bytes read. If data is NULL and
 *							max_data_len is 0 the number of bytes
 *							available for read should be returned.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len, uint32_t *read_bytes)
{
	struct tlx_usbd_ep_ctx *ep_ctx;
	struct tlx_usbd_ctx *ctx = get_usbd_ctx();
	uint32_t bytes_to_copy;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		return -EINVAL;
	}

	if (!data && max_data_len) {
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		LOG_ERR("Endpoint 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	k_mutex_lock(&ctx->drv_lock, K_FOREVER);
	bytes_to_copy = MIN(max_data_len, ep_ctx->buf.total_len);
	memcpy(data, ep_ctx->buf.data, bytes_to_copy);
	k_mutex_unlock(&ctx->drv_lock);
	*read_bytes = bytes_to_copy;
	return 0;
}

/**
 * @brief Continue reading data from the endpoint
 *
 * @details Clear the endpoint NAK and enable the endpoint to accept more data from
 * the host. Usually called after usb_dc_ep_read_wait() when the consumer is fine
 * to accept more data. Thus these calls together act as a flow control mechanism.
 *
 * @param[in]  ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_ep_read_continue(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached() || !dev_ready()) {
		return -ENODEV;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		return -EINVAL;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	if (!ep_ctx->cfg.en) {
		LOG_ERR("Ep 0x%02x is not enabled", ep);
		return -EINVAL;
	}

	LOG_DBG("Continue read ep 0x%02x", ep);

	if (ep_ctx->cfg.out_ack == false) {
		usb0hw_read_ep_data(USB_EP_GET_IDX(ep), ep_ctx->buf.data, ep_ctx->cfg.max_sz);
		ep_ctx->cfg.out_ack = true;
	}
	return 0;
}

/**
 * @brief Get endpoint max packet size
 *
 * @param[in]  ep	Endpoint address corresponding to the one
 *					listed in the device configuration table
 *
 * @return Endpoint max packet size (mps)
 */
int usb_dc_ep_mps(uint8_t ep)
{
	struct tlx_usbd_ep_ctx *ep_ctx;

	if (!dev_attached()) {
		return -ENODEV;
	}

	ep_ctx = endpoint_ctx(ep);
	if (!ep_ctx) {
		return -EINVAL;
	}

	return ep_ctx->cfg.max_sz;
}

/**
 * @brief Start the host wake up procedure.
 *
 * @details Function to wake up the host if it's currently in sleep mode.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_dc_wakeup_request(void)
{
	LOG_DBG("Remote wakeup");
	usb0hw_remote_wakeup();
	return 0;
}

static void usbd_work_handler(struct k_work *item)
{
	struct tlx_usbd_ctx *ctx;
	struct tlx_usbd_ep_ctx *ep_ctx;
	struct usbd_event *ev;

	ctx = CONTAINER_OF(item, struct tlx_usbd_ctx, usb_work);
	while ((ev = usbd_evt_get()) != NULL) {
		if (!dev_ready()) {
			usbd_evt_free(ev);
			LOG_DBG("USBD is not ready, event drops.");
			continue;
		}

		switch (ev->evt_type) {
		case USBD_EVT_IN_COMPLETE:
			LOG_DBG("IN_COMPLETE");
			usb_event_in_handler(ev->ep_addr);
			break;

		case USBD_EVT_EP_WRITE_COMPLETE:
			LOG_DBG("EP_WRITE_COMPLETE(0X%x)", ev->ep_addr);
			usb_event_ep_write_complete_handler(ev->ep_addr);
			break;

		case USBD_EVT_EP_RETRY:
			LOG_DBG("EP_RETRY");
			usb_event_ep_retry_handler(ev->ep_addr);
			break;

		case USBD_EVT_OUT_COMPLETE:
			LOG_DBG("OUT_COMPLETE");
			usb_event_out_complete_handler(ev->ep_addr);
			break;

		case USBD_EVT_OUT_SETUP:
			LOG_DBG("OUT_SETUP");
			usb_event_out_setup_handler(ev->ep_addr);
			break;

		case USBD_EVT_OUT_RCVD:
			LOG_DBG("OUT_RCVD(0X%x)", ev->ep_addr);
			usb_event_out_rcvd_handler(ev->ep_addr);
			break;

		case USBD_EVT_SUSPEND:
			LOG_DBG("SUSPEND");
			usb_event_suspend_handler();
			break;

		case USBD_EVT_WAKEUP:
			LOG_DBG("WAKEUP");
			usb_event_wakeup_handler();
			break;

		case USBD_EVT_RESET:
			LOG_DBG("RESET");
			usb_event_reset_handler();
			break;

		case USBD_EVT_REINIT:
			LOG_DBG("REINIT");
			break;

		default:
			LOG_ERR("Unknown USBD event: %" PRId16, ev->evt_type);
			break;
		}

		usbd_evt_free(ev);
	}
}

static void usbd_retry_timer_expire(struct k_timer *timer)
{
	struct tlx_usbd_ep_ctx *ep_ctx = k_timer_user_data_get(timer);

	submit_usbd_event(USBD_EVT_EP_RETRY, ep_ctx - usbd_ctx.ep_ctx);
}

static int usb_init(void)
{
	int ret;

#if CONFIG_USB_DC_HAS_HS_SUPPORT
	usb0hw_init(USB0_SPEED_HIGH);
#else
	usb0hw_init(USB0_SPEED_FULL);
#endif

	usb0hw_set_grxfsiz(EPS_BUFFER_OUT_SIZE);

	eps_buf_inf.seg_addr = EPS_BUFFER_OUT_SIZE;

	for (size_t i = 0; i < USBD_EP_TOTAL_CNT; i++) {
		k_timer_init(&usbd_ctx.ep_ctx[i].retry_timer, usbd_retry_timer_expire, NULL);
		k_timer_user_data_set(&usbd_ctx.ep_ctx[i].retry_timer, &usbd_ctx.ep_ctx[i]);
	}

	ret = usb_irq_init();
	k_work_queue_start(&usbd_work_queue, usbd_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(usbd_work_queue_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);

	k_work_init(&get_usbd_ctx()->usb_work, usbd_work_handler);

	return ret;
}

SYS_INIT(usb_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
