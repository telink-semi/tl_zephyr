/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NDS_MCACHE_CTL               0x7ca
#define NDS_MMISC_CTL                0x7d0
#define NDS_UITB                     0x800
#define PLIC_BASE_ADDRESS            DT_REG_ADDR(DT_NODELABEL(plic0))
#define PLIC_PRI_SIZE                ((CONFIG_NUM_IRQS - CONFIG_2ND_LVL_ISR_TBL_OFFSET) << 2)

#include <zephyr/toolchain.h>
#include <zephyr/devicetree.h>

	.option push
	.option norelax

/* exports */
GTEXT(entry)

SECTION_FUNC(init, init)
	.org 0x0
entry:

	j start

    .org 0x08
    .word   (0x544c4e4b)

	.word   (0x00880000 + _ramcode_size_div_16_align_256_)
	.word   (_RETENTION_DATA_VMA_START)

	.org 0x14
	.word   (0x00008278)

	# not need to add
	# .org 0x18
    # .word   (_bin_size_) 

	.option pop

	.org 0x20
    .global start_suspend
    .type start_suspend,@function

start_suspend:
    addi sp,sp,-16
    sw   a0,0(sp)
    sw   a1,4(sp)
    lui a1,0x800
    li  a0,0x81
    sb  a0,111(a1)

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    lw   a0,0(sp)
    lw   a1,4(sp)
    addi sp,sp,16
    ret

	.option push
	.option norelax

	.align 2

start:
	# /* not need to initialize global pointer, controlled by zephyr */
    # .option push
    # .option norelax
    # la     gp, __global_pointer$
    # .option pop

	/* not need to initialize stack pointer, controlled by zephyr */
    # la     t0, _stack_top_
    # mv     sp, t0

	/* not need to initialize machine non-vectored trap Base, controlled by zephyr */
    # la     t0, trap_entry
    # csrw   mtvec, t0

	/* not need to initialize plic interrupt priority, controlled by zephyr */
	# lui    t5, 0x860
	# mv     t4, t5
	# addi   t4, t4, 0xa0
	# addi   t5, t5, 4
# plic_loop_start:
#     bleu   t4, t5, plic_loop_end
#     li     t0, 1
#     sw     t0, 0(t5)
#     addi   t5, t5, 4
#     j      plic_loop_start
# plic_loop_end:

	/* not need to care for now */
# #if BLC_PM_DEEP_RETENTION_MODE_EN
# FLASH_WAKEUP_BEGIN:
#     lui     t0,0x800
#     addi    t0, t0, 0x0c
#     li      t1, 0x0
#     sb      t1, 0x1(t0)
#     li      t1, 0xab
#     sb      t1, 0x0(t0)           /*Flash deep cmd: 0xAB*/
#     li      t2, 0x0
#     li      t3, 0x10

# TNOP:
#     addi    t2, t2, 0x1
#     bleu    t3, t2, TNOP
#     li      t1, 0x1
#     sb      t1, 0x1(t0)
# FLASH_WAKEUP_END:
# #endif

	/* ic tag initialization */
ZERO_TAG:
	j ZERO_TAG
    lui    t0, 0
    la     t2, _ictag_start_
    la     t3, _ictag_end_
ZERO_TAG_BEGIN:
    bleu   t3, t2,_SET_IC
    sw     t0, 0(t2)
    addi   t2,t2,4
    j      ZERO_TAG_BEGIN
ZERO_TAG_END:

/* ic cache initialization */
_SET_IC:
	la     t0, _ramcode_size_align_256_
	srli   t0,t0,8

    la     t2, 0x80060c
    sb     t0, (t2)

	addi   t0, t0, 1        /* t0 = t0 + 1*/
	la     t2, 0x80060d
    sb     t0, 0(t2)
SET_IC_END:

/* retention data initialization */
COPY_RET_DATA:
    la     t1, _RETENTION_DATA_LMA_START
    la     t2, _RETENTION_DATA_VMA_START
    la     t3, _RETENTION_DATA_VMA_END
COPY_RET_DATA_BEGIN:
    bleu   t3, t2, _START
    lw     t0, 0(t1)
    sw     t0, 0(t2)
    addi   t1, t1, 4
    addi   t2, t2, 4
    j      COPY_RET_DATA_BEGIN
COPY_RET_DATA_END:

_START:
	j      __start

.option pop
