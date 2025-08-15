/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/syscall.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/syscall.h>
#include <../arch/riscv/core/asm_macros.inc>

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
#include <soc_isr_stacking.h>
#endif

/* Convenience macro for loading/storing register states. */
#define DO_CALLER_SAVED(op) \
	RV_E(	op t0, __z_arch_esf_t_t0_OFFSET(sp)	);\
	RV_E(	op t1, __z_arch_esf_t_t1_OFFSET(sp)	);\
	RV_E(	op t2, __z_arch_esf_t_t2_OFFSET(sp)	);\
	RV_I(	op t3, __z_arch_esf_t_t3_OFFSET(sp)	);\
	RV_I(	op t4, __z_arch_esf_t_t4_OFFSET(sp)	);\
	RV_I(	op t5, __z_arch_esf_t_t5_OFFSET(sp)	);\
	RV_I(	op t6, __z_arch_esf_t_t6_OFFSET(sp)	);\
	RV_E(	op a0, __z_arch_esf_t_a0_OFFSET(sp)	);\
	RV_E(	op a1, __z_arch_esf_t_a1_OFFSET(sp)	);\
	RV_E(	op a2, __z_arch_esf_t_a2_OFFSET(sp)	);\
	RV_E(	op a3, __z_arch_esf_t_a3_OFFSET(sp)	);\
	RV_E(	op a4, __z_arch_esf_t_a4_OFFSET(sp)	);\
	RV_E(	op a5, __z_arch_esf_t_a5_OFFSET(sp)	);\
	RV_I(	op a6, __z_arch_esf_t_a6_OFFSET(sp)	);\
	RV_I(	op a7, __z_arch_esf_t_a7_OFFSET(sp)	);\
	RV_E(	op ra, __z_arch_esf_t_ra_OFFSET(sp)	)

	.macro get_current_cpu dst
#if defined(CONFIG_SMP) || defined(CONFIG_USERSPACE)
	csrr \dst, mscratch
#else
	la \dst, _kernel + ___kernel_t_cpus_OFFSET
#endif
	.endm

/* imports */
GTEXT(_irq_vector_handler)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

GTEXT(z_get_next_switch_handle)
GTEXT(z_riscv_switch)

#ifdef CONFIG_TRACING
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif

/* exports */
GTEXT(_isr_vectored_wrapper)

/*
 * Handler called upon each exception/interrupt/fault
 */
SECTION_FUNC(exception.entry, _isr_vectored_wrapper)

#ifdef CONFIG_USERSPACE
	/* retrieve address of _current_cpu preserving s0 */
	csrrw s0, mscratch, s0

	/* preserve t0 and t1 temporarily */
	sr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	sr t1, _curr_cpu_arch_user_exc_tmp1(s0)

	/* determine if we come from user space */
	csrr t0, mstatus
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f

	/* in user space we were: switch to our privileged stack */
	mv t0, sp
	lr sp, _curr_cpu_arch_user_exc_sp(s0)

	/* Save user stack value. Coming from user space, we know this
	 * can't overflow the privileged stack. The esf will be allocated
	 * later but it is safe to store our saved user sp here. */
	sr t0, (-__z_arch_esf_t_SIZEOF + __z_arch_esf_t_sp_OFFSET)(sp)

	/* Make sure tls pointer is sane */
	lr t0, ___cpu_t_current_OFFSET(s0)
	lr tp, _thread_offset_to_tls(t0)

	/* Clear our per-thread usermode flag */
	lui t0, %tprel_hi(is_user_mode)
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb zero, %tprel_lo(is_user_mode)(t0)
1:
	/* retrieve original t0/t1 values */
	lr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	lr t1, _curr_cpu_arch_user_exc_tmp1(s0)

	/* retrieve original s0 and restore _current_cpu in mscratch */
	csrrw s0, mscratch, s0
#endif

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
	SOC_ISR_SW_STACKING
#else
	/* Save caller-saved registers on current thread stack. */
	addi sp, sp, -__z_arch_esf_t_SIZEOF
	DO_CALLER_SAVED(sr)		;
#endif /* CONFIG_RISCV_SOC_HAS_ISR_STACKING */

	/* Save s0 in the esf and load it with &_current_cpu. */
	sr s0, __z_arch_esf_t_s0_OFFSET(sp)
	get_current_cpu s0

	/* Save MEPC register */
	csrr t0, mepc
	sr t0, __z_arch_esf_t_mepc_OFFSET(sp)

	/* Save MSTATUS register */
	csrr t2, mstatus
	sr t2, __z_arch_esf_t_mstatus_OFFSET(sp)

#if defined(CONFIG_FPU_SHARING)
	/* determine if FPU access was disabled */
	li t1, MSTATUS_FS
	and t1, t1, t2
	bnez t1, no_fp
	/* determine if this is an Illegal Instruction exception */
	csrr t2, mcause
	li t1, 2		/* 2 = illegal instruction */
	bne t1, t2, no_fp
	/* determine if we trapped on an FP instruction. */
	csrr t2, mtval		/* get faulting instruction */
#ifdef CONFIG_QEMU_TARGET
	/*
	 * Some implementations may not support MTVAL in this capacity.
	 * Notably QEMU when a CSR instruction is involved.
	 */
	bnez t2, 1f
	lw t2, 0(t0)		/* t0 = mepc */
1:
#endif
	andi t0, t2, 0x7f	/* keep only the opcode bits */
	xori t1, t0, 0b1010011	/* OP-FP */
	beqz t1, is_fp
	ori  t0, t0, 0b0100000
	xori t1, t0, 0b0100111	/* LOAD-FP / STORE-FP */
	beqz t1, is_fp
	/*
	 * The FRCSR, FSCSR, FRRM, FSRM, FSRMI, FRFLAGS, FSFLAGS and FSFLAGSI
	 * are in fact CSR instructions targeting the fcsr, frm and fflags
	 * registers. They should be caught as FPU instructions as well.
	 *
	 * CSR format: csr#[31-20] src[19-15] op[14-12] dst[11-7] SYSTEM[6-0]
	 * SYSTEM = 0b1110011, op = 0b.xx where xx is never 0
	 * The csr# of interest are: 1=fflags, 2=frm, 3=fcsr
	 */
	xori t1, t0, 0b1110011	/* SYSTEM opcode */
	bnez t1, 2f		/* not a CSR insn */
	srli t0, t2, 12
	andi t0, t0, 0x3
	beqz t0, 2f		/* not a CSR insn */
	srli t0, t2, 20		/* isolate the csr register number */
	beqz t0, 2f		/* 0=ustatus */
	andi t0, t0, ~0x3	/* 1=fflags, 2=frm, 3=fcsr */
#if !defined(CONFIG_RISCV_ISA_EXT_C)
	bnez t0, no_fp
#else
	beqz t0, is_fp
2:	/* remaining non RVC (0b11) and RVC with 0b01 are not FP instructions */
	andi t1, t2, 1
	bnez t1, no_fp
	/*
	 * 001...........00 = C.FLD    RV32/64  (RV128 = C.LQ)
	 * 001...........10 = C.FLDSP  RV32/64  (RV128 = C.LQSP)
	 * 011...........00 = C.FLW    RV32     (RV64/128 = C.LD)
	 * 011...........10 = C.FLWSPP RV32     (RV64/128 = C.LDSP)
	 * 101...........00 = C.FSD    RV32/64  (RV128 = C.SQ)
	 * 101...........10 = C.FSDSP  RV32/64  (RV128 = C.SQSP)
	 * 111...........00 = C.FSW    RV32     (RV64/128 = C.SD)
	 * 111...........10 = C.FSWSP  RV32     (RV64/128 = C.SDSP)
	 *
	 * so must be .01............. on RV64 and ..1............. on RV32.
	 */
	srli t0, t2, 8
#if defined(CONFIG_64BIT)
	andi t1, t0, 0b01100000
	xori t1, t1, 0b00100000
	bnez t1, no_fp
#else
	andi t1, t0, 0b00100000
	beqz t1, no_fp
#endif
#endif /* CONFIG_RISCV_ISA_EXT_C */

is_fp:	/* Process the FP trap and quickly return from exception */
	la ra, fp_trap_exit
	mv a0, sp
	tail z_riscv_fpu_trap
2:
no_fp:	/* increment _current->arch.exception_depth */
	lr t0, ___cpu_t_current_OFFSET(s0)
	lb t1, _thread_offset_to_exception_depth(t0)
	add t1, t1, 1
	sb t1, _thread_offset_to_exception_depth(t0)

	/* configure the FPU for exception mode */
	call z_riscv_fpu_enter_exc
#endif /* CONFIG_FPU_SHARING */

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_save_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#ifdef CONFIG_PMP_STACK_GUARD
#ifdef CONFIG_USERSPACE
	/*
	 * If we came from userspace then we need to reconfigure the
	 * PMP for kernel mode stack guard.
	 */
	lr t0, __z_arch_esf_t_mstatus_OFFSET(sp)
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
	j 2f
#endif /* CONFIG_USERSPACE */
1:	/* Re-activate PMP for m-mode */
	li t1, MSTATUS_MPP
	csrc mstatus, t1
	li t1, MSTATUS_MPRV
	csrs mstatus, t1
2:
#endif

	/* Increment _current_cpu->nested */
	lw t1, ___cpu_t_nested_OFFSET(s0)
	addi t2, t1, 1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t1, on_irq_stack

	/* Switch to interrupt stack */
	mv t0, sp
	lr sp, ___cpu_t_irq_stack_OFFSET(s0)

	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	addi sp, sp, -16
	sr t0, 0(sp)

on_irq_stack:

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_enter
#endif

	/* Get IRQ causing interrupt */
	csrr a0, mcause
	li t0, SOC_MCAUSE_EXP_MASK
	and a0, a0, t0

	call _irq_vector_handler

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_exit
#endif

irq_done:
	/* Decrement _current_cpu->nested */
	lw t2, ___cpu_t_nested_OFFSET(s0)
	addi t2, t2, -1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t2, no_reschedule

	/* nested count is back to 0: Return to thread stack */
	lr sp, 0(sp)

#ifdef CONFIG_STACK_SENTINEL
	call z_check_stack_sentinel
#endif

check_reschedule:

	/* Get pointer to current thread on this CPU */
	lr a1, ___cpu_t_current_OFFSET(s0)

	/*
	 * Get next thread to schedule with z_get_next_switch_handle().
	 * We pass it a NULL as we didn't save the whole thread context yet.
	 * If no scheduling is necessary then NULL will be returned.
	 */
	addi sp, sp, -16
	sr a1, 0(sp)
	mv a0, zero
	call z_get_next_switch_handle
	lr a1, 0(sp)
	addi sp, sp, 16
	beqz a0, no_reschedule

reschedule:

	/*
	 * Perform context switch:
	 * a0 = new thread
	 * a1 = old thread
	 */
	call z_riscv_switch

might_have_rescheduled:
	/* reload s0 with &_current_cpu as it might have changed or be unset */
	get_current_cpu s0

no_reschedule:

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#if defined(CONFIG_FPU_SHARING)
	/* FPU handling upon exception mode exit */
	mv a0, sp
	call z_riscv_fpu_exit_exc

	/* decrement _current->arch.exception_depth */
	lr t0, ___cpu_t_current_OFFSET(s0)
	lb t1, _thread_offset_to_exception_depth(t0)
	add t1, t1, -1
	sb t1, _thread_offset_to_exception_depth(t0)
fp_trap_exit:
#endif

	/* Restore MEPC and MSTATUS registers */
	lr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	lr t2, __z_arch_esf_t_mstatus_OFFSET(sp)
	csrw mepc, t0
	csrw mstatus, t2

#ifdef CONFIG_USERSPACE
	/*
	 * Check if we are returning to user mode. If so then we must
	 * set is_user_mode to true and preserve our kernel mode stack for
	 * the next exception to come.
	 */
	li t1, MSTATUS_MPP
	and t0, t2, t1
	bnez t0, 1f

#ifdef CONFIG_PMP_STACK_GUARD
	/* Remove kernel stack guard and Reconfigure PMP for user mode */
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_usermode_enable
#endif

	/* Set our per-thread usermode flag */
	li t1, 1
	lui t0, %tprel_hi(is_user_mode)
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb t1, %tprel_lo(is_user_mode)(t0)

	/* preserve stack pointer for next exception entry */
	add t0, sp, __z_arch_esf_t_SIZEOF
	sr t0, _curr_cpu_arch_user_exc_sp(s0)

	j 2f
1:
	/*
	 * We are returning to kernel mode. Store the stack pointer to
	 * be re-loaded further down.
	 */
	addi t0, sp, __z_arch_esf_t_SIZEOF
	sr t0, __z_arch_esf_t_sp_OFFSET(sp)
2:
#endif

	/* Restore s0 (it is no longer ours) */
	lr s0, __z_arch_esf_t_s0_OFFSET(sp)

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
	SOC_ISR_SW_UNSTACKING
#else
	/* Restore caller-saved registers from thread stack */
	DO_CALLER_SAVED(lr)

#ifdef CONFIG_USERSPACE
	/* retrieve saved stack pointer */
	lr sp, __z_arch_esf_t_sp_OFFSET(sp)
#else
	/* remove esf from the stack */
	addi sp, sp, __z_arch_esf_t_SIZEOF
#endif

#endif /* CONFIG_RISCV_SOC_HAS_ISR_STACKING */

	mret
