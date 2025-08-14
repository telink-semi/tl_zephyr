/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

#include "soc.h"

GTEXT(pm_w91_sleep)
GTEXT(pm_w91_wakeup)

SECTION_VAR(ram_code, pm_w91_context_backup_registers)
	.skip 128 * 4
/******************************************
 *  0 -  x1 - ra
 *  1 -  x2 - sp
 *  2 -  x3 - gp
 *  3 -  x4 - tp
 *  4 -  x5 - t0
 *  5 -  x6 - t1
 *  6 -  x7 - t2
 *  7 -  x8 - s0/fp
 *  8 -  x9 - s1
 *  9 - x10 - a0
 * 10 - x11 - a1
 * 11 - x12 - a2
 * 12 - x13 - a3
 * 13 - x14 - a4
 * 14 - x15 - a5
 * 15 - x16 - a6
 * 16 - x17 - a7
 * 17 - x18 - s2
 * 18 - x19 - s3
 * 19 - x20 - s4
 * 20 - x21 - s5
 * 21 - x22 - s6
 * 22 - x23 - s7
 * 23 - x24 - s8
 * 24 - x25 - s9
 * 25 - x26 - s10
 * 26 - x27 - s11
 * 27 - x28 - t3
 * 28 - x29 - t4
 * 29 - x30 - t5
 * 30 - x31 - t6
 * 31 - mepc
 * 32 - mcause
 * 33 - mtval
 * 34 - mscratch
 * 35 - mtvec
 * 36 - mmisc
 * 37 - mstatus
 * 38 - mxstatus
 * 39 - mdcause
 * 40 - mip
 * 41 - mie
 * 42 -  f0 - ft0
 * 43 -  f1 - ft1
 * 44 -  f2 - ft2
 * 45 -  f3 - ft3
 * 46 -  f4 - ft4
 * 47 -  f5 - ft5
 * 48 -  f6 - ft6
 * 49 -  f7 - ft7
 * 50 -  f8 - fs0
 * 51 -  f9 - fs1
 * 52 - f10 - fa0
 * 53 - f11 - fa1
 * 54 - f12 - fa2
 * 55 - f13 - fa3
 * 56 - f14 - fa4
 * 57 - f15 - fa5
 * 58 - f16 - fa6
 * 59 - f17 - fa7
 * 60 - f18 - fs2
 * 61 - f19 - fs3
 * 62 - f20 - fs4
 * 63 - f21 - fs5
 * 64 - f22 - fs6
 * 65 - f23 - fs7
 * 66 - f24 - fs8
 * 67 - f25 - fs9
 * 68 - f26 - fs10
 * 69 - f27 - fs11
 * 70 - f28 - ft8
 * 71 - f29 - ft9
 * 72 - f30 - ft10
 * 73 - f31 - ft11
 * 74 - fcsr
 * *****************************************/

/****************************************************************************
 * Name: pm_save_context
 ****************************************************************************/

.macro pm_save_context

	addi   sp, sp, (-2 * 4)
	sw     x30, (1 * 4)(sp)
	sw     x31, (2 * 4)(sp)

	lui    x31, % hi(pm_w91_context_backup_registers)
	addi   x31, x31, % lo(pm_w91_context_backup_registers)

	sw     x1, (0 * 4)(x31)
	sw     x3, (2 * 4)(x31)
	sw     x4, (3 * 4)(x31)
	sw     x5, (4 * 4)(x31)
	sw     x6, (5 * 4)(x31)
	sw     x7, (6 * 4)(x31)
	sw     x8, (7 * 4)(x31)
	sw     x9, (8 * 4)(x31)
	sw     x10, (9 * 4)(x31)
	sw     x11, (10 * 4)(x31)
	sw     x12, (11 * 4)(x31)
	sw     x13, (12 * 4)(x31)
	sw     x14, (13 * 4)(x31)
	sw     x15, (14 * 4)(x31)
	sw     x16, (15 * 4)(x31)
	sw     x17, (16 * 4)(x31)
	sw     x18, (17 * 4)(x31)
	sw     x19, (18 * 4)(x31)
	sw     x20, (19 * 4)(x31)
	sw     x21, (20 * 4)(x31)
	sw     x22, (21 * 4)(x31)
	sw     x23, (22 * 4)(x31)
	sw     x24, (23 * 4)(x31)
	sw     x25, (24 * 4)(x31)
	sw     x26, (25 * 4)(x31)
	sw     x27, (26 * 4)(x31)
	sw     x28, (27 * 4)(x31)
	sw     x29, (28 * 4)(x31)
	sw     x30, (29 * 4)(x31)

	move   x30, x31

	/* save x2 - sp */
	move   x31, x2
	addi   x31, x31, (2 * 4)
	sw     x31, (1 * 4)(x30)

	/* save csrs */
	csrr   x31, mepc
	sw     x31, (31 * 4)(x30)

	csrr   x31, mcause
	sw     x31, (32 * 4)(x30)

	csrr   x31, mtval
	sw     x31, (33 * 4)(x30)

	csrr   x31, mscratch
	sw     x31, (34 * 4)(x30)

	csrr   x31, mtvec
	sw     x31, (35 * 4)(x30)

	csrr   x31, NDS_MMISC_CTL
	sw     x31, (36 * 4)(x30)

	csrr   x31, mstatus
	sw     x31, (37 * 4)(x30)

	csrr   x31, NDS_MXSTATUS
	sw     x31, (38 * 4)(x30)

	csrr   x31, NDS_MDCAUSE
	sw     x31, (39 * 4)(x30)

	csrr   x31, mip
	sw     x31, (40 * 4)(x30)

	csrr   x31, mie
	sw     x31, (41 * 4)(x30)

#ifdef __riscv_flen
	frcsr  x31
	fsw    f0, (42 * 4)(x30)
	fsw    f1, (43 * 4)(x30)
	fsw    f2, (44 * 4)(x30)
	fsw    f3, (45 * 4)(x30)
	fsw    f4, (46 * 4)(x30)
	fsw    f5, (47 * 4)(x30)
	fsw    f6, (48 * 4)(x30)
	fsw    f7, (49 * 4)(x30)
	fsw    f8, (50 * 4)(x30)
	fsw    f9, (51 * 4)(x30)
	fsw    f10, (52 * 4)(x30)
	fsw    f11, (53 * 4)(x30)
	fsw    f12, (54 * 4)(x30)
	fsw    f13, (55 * 4)(x30)
	fsw    f14, (56 * 4)(x30)
	fsw    f15, (57 * 4)(x30)
	fsw    f16, (58 * 4)(x30)
	fsw    f17, (59 * 4)(x30)
	fsw    f18, (60 * 4)(x30)
	fsw    f19, (61 * 4)(x30)
	fsw    f20, (62 * 4)(x30)
	fsw    f21, (63 * 4)(x30)
	fsw    f22, (64 * 4)(x30)
	fsw    f23, (65 * 4)(x30)
	fsw    f24, (66 * 4)(x30)
	fsw    f25, (67 * 4)(x30)
	fsw    f26, (68 * 4)(x30)
	fsw    f27, (69 * 4)(x30)
	fsw    f28, (70 * 4)(x30)
	fsw    f29, (71 * 4)(x30)
	fsw    f30, (72 * 4)(x30)
	fsw    f31, (73 * 4)(x30)
	sw     x31, (74 * 4)(x30)
#endif

	/* save x31 - t6 */
	lw     x31, (2 * 4)(sp)
	sw     x31, (30 * 4)(x30)

	lw     x30, (1 * 4)(sp)
	addi   sp, sp, (2 * 4)

.endm

/****************************************************************************
 * Name: pm_restore_context
 ****************************************************************************/

.macro pm_restore_context

	lui    x31, % hi(pm_w91_context_backup_registers)
	addi   x31, x31, % lo(pm_w91_context_backup_registers)

	/* restore x2 - sp */
	lw     x2, (1 * 4)(x31)

	/* restore csrs */
	lw     x30, (31 * 4)(x31)
	csrw   mepc, x30

	lw     x30, (32 * 4)(x31)
	csrw   mcause, x30

	lw     x30, (33 * 4)(x31)
	csrw   mtval, x30

	lw     x30, (34 * 4)(x31)
	csrw   mscratch, x30

	lw     x30, (35 * 4)(x31)
	csrw   mtvec, x30

	lw     x30, (36 * 4)(x31)
	csrw   NDS_MMISC_CTL, x30

	lw     x30, (37 * 4)(x31)
	csrw   mstatus, x30

	lw     x30, (38 * 4)(x31)
	csrw   NDS_MXSTATUS, x30

	lw     x30, (39 * 4)(x31)
	csrw   NDS_MDCAUSE, x30

	lw     x30, (40 * 4)(x31)
	csrw   mip, x30

	lw     x30, (41 * 4)(x31)
	csrw   mie, x30

#ifdef __riscv_flen
	flw    f0, (42 * 4)(x31)
	flw    f1, (43 * 4)(x31)
	flw    f2, (44 * 4)(x31)
	flw    f3, (45 * 4)(x31)
	flw    f4, (46 * 4)(x31)
	flw    f5, (47 * 4)(x31)
	flw    f6, (48 * 4)(x31)
	flw    f7, (49 * 4)(x31)
	flw    f8, (50 * 4)(x31)
	flw    f9, (51 * 4)(x31)
	flw    f10, (52 * 4)(x31)
	flw    f11, (53 * 4)(x31)
	flw    f12, (54 * 4)(x31)
	flw    f13, (55 * 4)(x31)
	flw    f14, (56 * 4)(x31)
	flw    f15, (57 * 4)(x31)
	flw    f16, (58 * 4)(x31)
	flw    f17, (59 * 4)(x31)
	flw    f18, (60 * 4)(x31)
	flw    f19, (61 * 4)(x31)
	flw    f20, (62 * 4)(x31)
	flw    f21, (63 * 4)(x31)
	flw    f22, (64 * 4)(x31)
	flw    f23, (65 * 4)(x31)
	flw    f24, (66 * 4)(x31)
	flw    f25, (67 * 4)(x31)
	flw    f26, (68 * 4)(x31)
	flw    f27, (69 * 4)(x31)
	flw    f28, (70 * 4)(x31)
	flw    f29, (71 * 4)(x31)
	flw    f30, (72 * 4)(x31)
	flw    f31, (73 * 4)(x31)
	lw     x30, (74 * 4)(x31)
	fscsr  x30
#endif

	lw     x1, (0 * 4)(x31)
	lw     x3, (2 * 4)(x31)
	lw     x4, (3 * 4)(x31)
	lw     x5, (4 * 4)(x31)
	lw     x6, (5 * 4)(x31)
	lw     x7, (6 * 4)(x31)
	lw     x8, (7 * 4)(x31)
	lw     x9, (8 * 4)(x31)
	lw     x10, (9 * 4)(x31)
	lw     x11, (10 * 4)(x31)
	lw     x12, (11 * 4)(x31)
	lw     x13, (12 * 4)(x31)
	lw     x14, (13 * 4)(x31)
	lw     x15, (14 * 4)(x31)
	lw     x16, (15 * 4)(x31)
	lw     x17, (16 * 4)(x31)
	lw     x18, (17 * 4)(x31)
	lw     x19, (18 * 4)(x31)
	lw     x20, (19 * 4)(x31)
	lw     x21, (20 * 4)(x31)
	lw     x22, (21 * 4)(x31)
	lw     x23, (22 * 4)(x31)
	lw     x24, (23 * 4)(x31)
	lw     x25, (24 * 4)(x31)
	lw     x26, (25 * 4)(x31)
	lw     x27, (26 * 4)(x31)
	lw     x28, (27 * 4)(x31)
	lw     x29, (28 * 4)(x31)
	lw     x30, (29 * 4)(x31)
	lw     x31, (29 * 4)(x31)

.endm

/****************************************************************************
 * Name: pm_w91_sleep
 ****************************************************************************/

.align 2
SECTION_FUNC(ram_code, pm_w91_sleep)

	pm_save_context
	wfi

	/* There are 3 possible case after etner wfi
	 * a) nothing happens from N22, and D25 irq (mtimcmp) cause continuation
	 * b) N22 enter SLEEP/IDLE, wakeup, restore, and signal D25 using PM SIG
	 * c) N22 enter DEEPSLEEP/HIBER, wakeup, restore, and make D25 jump
	 *    to pm_sleep_leave
	 */
	ret

/****************************************************************************
 * Name: pm_w91_wakeup
 ****************************************************************************/

.align 2
SECTION_FUNC(ram_code, pm_w91_wakeup)

	pm_restore_context
	ret
