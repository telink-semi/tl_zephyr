/** @file app_fifo.h
 *  @brief
 */

/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __APP_FIFO_H__
#define __APP_FIFO_H__
#include "types.h"
#include "tl_string.h"

/**
 * @brief   Callback function type for FIFO frame processing result
 *
 * This callback is invoked after a FIFO frame has been processed by the upper
 * layer (e.g., transmitted successfully over 2.4G RF, or failed after all
 * retries). It allows the caller to perform follow-up operations such as
 * resource release, state update, logging, or triggering application events.
 *
 * @param[in] data      Pointer to the payload associated with the processed frame
 * @param[in] len       Length of the payload
 * @param[in] success   Processing result:
 *                      - true  : The frame was processed/sent successfully  
 *                      - false : The frame failed permanently (e.g., retry exceeded)
 * @param[in] user_arg  User-defined argument passed during registration or FIFO push
 *
 * @return    None
 *
 * @note Users should avoid heavy operations inside the callback.
 */
typedef void (*fifo_cb_t)(unsigned char *data, unsigned char len, bool success, void *user_arg);


/**
 * @brief   Generic FIFO structure for command/data buffering
 *
 * This structure defines a simple linear FIFO buffer used for storing
 * command frames or general data frames. It maintains read/write positions,
 * element size, buffer capacity, and an internal pointer to the data buffer.
 *
 * @param size   Size of each FIFO element (in bytes)
 * @param num    Total number of elements the FIFO can hold
 * @param wptr   Write pointer index (managed internally)
 * @param rptr   Read pointer index (managed internally)
 * @param p      Pointer to the FIFO memory buffer
 */
typedef struct
{
    unsigned short size;
    unsigned short num;

    unsigned short wptr;
    unsigned short rptr;

    unsigned char *p;
} pl_fifo_t;

/**
 * @brief   Reset FIFO read/write pointers
 *
 * This function resets the FIFO read and write pointers, clearing all
 * existing FIFO content without modifying the underlying memory buffer.
 *
 * @param[in] f     Pointer to a FIFO instance
 *
 * @return  None
 */
void pp_fifo_reset(pl_fifo_t *f);


/**
 * @brief   Get number of valid elements currently stored in the FIFO
 *
 * Calculates how many data frames are currently available in the FIFO.  
 * This is typically used before reading or popping elements to verify
 * whether sufficient data is available.
 *
 * @param[in] f     Pointer to a FIFO instance
 *
 * @return  Number of valid FIFO entries
 */
unsigned short pp_fifo_get_num(pl_fifo_t *f);


/**
 * @brief   Get pointer to the current read position
 *
 * Returns a pointer to the FIFO element at the current read pointer.
 * The caller may access the data directly, but must call `pp_fifo_pop()`
 * afterward to advance the read pointer.
 *
 * @param[in] f     Pointer to a FIFO instance
 *
 * @return  Pointer to the current FIFO element data
 */
unsigned char *pp_fifo_get_ptr(pl_fifo_t *f);


/**
 * @brief   Push a command + payload into the FIFO
 *
 * This function inserts one complete data frame into the FIFO.  
 * A typical frame format is:
 *     [cmd][len][payload...]
 *
 * @param[in] f      Pointer to a FIFO instance  
 * @param[in] cmd    Command byte  
 * @param[in] buf    Pointer to payload buffer  
 * @param[in] len    Length of the payload  
 *
 * @return  
 *      0  : success  
 *     -1 : FIFO full or operation failed  
 *
 * @note  The total frame size must not exceed FIFO element size.
 */
int pp_fifo_push(pl_fifo_t *f, unsigned char cmd, unsigned char *buf, unsigned char len);


/**
 * @brief   Pop (remove) the current FIFO element
 *
 * Advances the read pointer by one element.  
 * Caller must ensure FIFO is not empty before calling this function
 * (use `pp_fifo_is_empty()` or check `pp_fifo_get_num()`).
 *
 * @param[in] f     Pointer to a FIFO instance
 *
 * @return  None
 */
void pp_fifo_pop(pl_fifo_t *f);


/**
 * @brief   Check whether the FIFO is empty
 *
 * Determines whether the FIFO contains zero valid elements.
 *
 * @param[in] fifo    Pointer to a FIFO instance
 *
 * @return  
 *      true  : FIFO is empty  
 *      false : FIFO contains at least one element
 */
bool pp_fifo_is_empty(const pl_fifo_t *fifo);

/**
 * @brief   Check whether the message FIFO is full
 *
 * Determines if the FIFO has no available slot for new message.
 * This FIFO reserves 1 empty slot to distinguish between empty and full states.
 *
 * @param[in] fifo    Pointer to the FIFO instance
 *
 * @return
 *      true  : FIFO is full (cannot push new message)
 *      false : FIFO is not full (can push new message)
 */
bool pp_fifo_is_full(const pl_fifo_t *fifo);

/**
 * @brief   Get the number of remaining available slots in the message FIFO
 *
 * Calculates the number of available slots that can be used to store new messages.
 * The calculation takes into account that this FIFO reserves 1 empty slot to 
 * distinguish between empty and full states, so the actual usable slots is (num - 1).
 *
 * @param[in] fifo    Pointer to the FIFO instance
 *
 * @return
 *      Number of available slots that can accept new messages
 *      0 : FIFO is full (no available slots)
 *      Positive value : Number of available slots ready for new messages
 */
unsigned short pp_fifo_get_remaining_slots(pl_fifo_t *f);

/**
 * @brief   Push a command frame with extended metadata into the FIFO
 *
 * This function inserts a complete command/data frame into the FIFO, with
 * additional extended parameters such as retry count and a user-defined
 * callback. It is typically used in 2.4G protocol stacks when spp packet
 * is send.
 *
 * The pushed frame generally includes:
 *     [cmd][len][payload...][retry_num][callback][user_arg]
 *
 * @param[in] f          Pointer to a FIFO instance
 * @param[in] cmd        Command byte associated with this frame
 * @param[in] buf        Pointer to the payload buffer
 * @param[in] len        Length of the payload
 * @param[in] retry_num  Maximum retry count for this frame
 * @param[in] cb         Callback invoked when the frame is successfully sent
 *                       or definitively failed (type: @ref fifo_cb_t)
 * @param[in] user_arg   User-defined argument passed to the callback
 *
 * @return
 *      0   : Success  
 *     -1   : FIFO full or insufficient space  
 *     -2   : Invalid parameters  
 *
 * @note The total extended frame size must not exceed the FIFO element size.
 *       If a callback is provided, upper layers must ensure the callback
 *       context is safe to execute (typically invoked in task context).
 */
int pp_fifo_push_extra(pl_fifo_t *f, unsigned char cmd, unsigned char *buf, unsigned char len,
                       unsigned char retry_num, fifo_cb_t cb, void *user_arg);




#endif // __APP_FIFO_H__
