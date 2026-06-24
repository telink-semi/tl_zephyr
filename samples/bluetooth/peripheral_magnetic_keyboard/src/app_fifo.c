/** @file app_fifo.c
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include "app_fifo.h"
#include "app_common_config.h"

static struct k_spinlock pool_lock;  

_attribute_ram_code_sec_ void pp_fifo_reset (pl_fifo_t *f)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    f->wptr = 0;
    f->rptr = 0;
    k_spin_unlock(&pool_lock, key);
}

_attribute_ram_code_sec_ int pp_fifo_push(pl_fifo_t *f, unsigned char cmd, unsigned char *buf, unsigned char len)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    if (len > (f->size - 2))
    {
        k_spin_unlock(&pool_lock, key);
        return TLK_ERR_INVALID_LENGTH;
    }
    if (((f->wptr - f->rptr) & 255) < (f->num - 1))
    {

        unsigned char *pd = (unsigned char *)(f->p + (f->wptr & (f->num - 1)) * f->size);

        pd[0] = len;
        pd[1] = cmd;
        tmemcpy(&pd[2], &buf[0], len);
        f->wptr++;
        k_spin_unlock(&pool_lock, key);
        return TLK_SUCCESS;
    }
    k_spin_unlock(&pool_lock, key);
    return TLK_ERR_BUFFER_FULL;
}

 _attribute_ram_code_sec_ int pp_fifo_push_extra(pl_fifo_t *f, unsigned char cmd, unsigned char *buf, unsigned char len,
                                                 unsigned char retry_num, fifo_cb_t cb, void *user_arg)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    if (len > (f->size - 12))
    {
        k_spin_unlock(&pool_lock, key);
        return TLK_ERR_INVALID_LENGTH;
    }
    if (((f->wptr - f->rptr) & 255) < (f->num - 1))
    {

        unsigned char *pd = (unsigned char *)(f->p + (f->wptr & (f->num - 1)) * f->size);

        pd[0] = len;
        pd[1] = cmd;
        tmemcpy(&pd[2], &buf[0], len);

        *(fifo_cb_t *)(&pd[len + 2]) = cb;
        *(void **)(&pd[len + 6]) = user_arg;
        pd[len + 10] = retry_num;

        f->wptr++;
        k_spin_unlock(&pool_lock, key);
        return TLK_SUCCESS;
    }
    k_spin_unlock(&pool_lock, key);
    return TLK_ERR_BUFFER_FULL;
}

_attribute_ram_code_sec_ unsigned char *pp_fifo_get_ptr (pl_fifo_t *f)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    if (f->rptr != f->wptr)
    {
        unsigned char *p = f->p + (f->rptr & (f->num-1)) * f->size;
        k_spin_unlock(&pool_lock, key);
        return p;
    }
    k_spin_unlock(&pool_lock, key);
    return TLK_SUCCESS;
}


_attribute_ram_code_sec_ unsigned short pp_fifo_get_num(pl_fifo_t *f)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    unsigned short num =(f->wptr - f->rptr) & 255;
    k_spin_unlock(&pool_lock, key);
    return num;
}

_attribute_ram_code_sec_ void pp_fifo_pop(pl_fifo_t *f)
{
    k_spinlock_key_t key = k_spin_lock(&pool_lock);
    f->rptr++;
    k_spin_unlock(&pool_lock, key);
}

_attribute_ram_code_sec_ bool pp_fifo_is_empty(const pl_fifo_t *fifo)
{
    return (fifo->wptr == fifo->rptr);
}

_attribute_ram_code_sec_ bool pp_fifo_is_full(const pl_fifo_t *fifo)
{
    return (((fifo->wptr - fifo->rptr) & 255) >= (fifo->num - 1));
}

_attribute_ram_code_sec_ unsigned short pp_fifo_get_remaining_slots(pl_fifo_t *f)
{
    return ((f->num - 1) - ((f->wptr - f->rptr) & 255));
}

