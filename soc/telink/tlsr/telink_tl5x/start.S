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
#if CONFIG_SOC_RISCV_TELINK_TL322X
    .equ __D25F_IRAM_SIZE,   2 //64KB
    .equ __D25F_DRAM_SIZE,   3 //96KB
    .equ __N22_IRAM_SIZE,    5 //160KB
    .equ __N22_DRAM_SIZE,    2 //64KB

    .equ __D25F_N22_RAM_REG,    ((__N22_DRAM_SIZE+__N22_IRAM_SIZE)<<12)|(__N22_DRAM_SIZE<<8)|(__D25F_DRAM_SIZE)

    .section .vectors, "ax"

    .global     __D25F_IRAM_SIZE
    .global     __D25F_DRAM_SIZE
    .global     __N22_IRAM_SIZE
    .global     __N22_DRAM_SIZE
    .global     __D25F_N22_RAM_REG
#elif CONFIG_SOC_RISCV_TELINK_TL323X
    .equ __D25F_IRAM_SIZE,   3 //96KB   note: configuration Options: 1/2/3
    .equ __D25F_DRAM_SIZE,   2 //64KB   note: configuration Options: 4/3/2

	.global     __D25F_IRAM_SIZE
    .global     __D25F_DRAM_SIZE
#endif

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

	.org 0x20
	.word ('T' << 24 | 'L' << 16 | 'N' << 8 | 'K')

	.org 0x24
#if defined(CONFIG_TELINK_TLX_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_TLX_2_WIRE_SPI_ENABLE
	.word (0x3b4097a9)              #/ 2 - wire SPI interface
#else
	.word (0x3b4097a9)              #/ 4 - wire SPI interface set in soc_tlx_check_flash
#endif

	.align 2

start:
#if CONFIG_SOC_RISCV_TELINK_TL322X
    /*Initialize D25 & N22 ILM DLM Size */
    lui    t0 , 0x80141
    li     t1 , __D25F_N22_RAM_REG
    sh     t1 , -2036(t0)              //write16(0x8014080c,0x2105)
#elif CONFIG_SOC_RISCV_TELINK_TL323X
    /*Initialize D25 ILM DLM Size */
    lui    t0 , 0x80141
    li     t1 , __D25F_DRAM_SIZE
    sb     t1 , -2036(t0)              //write8(0x8014080c,0x2105)
#endif

#if defined(CONFIG_ANDES_CODENSE) && CONFIG_ANDES_CODENSE
	la     t0, _ITB_BASE_
	csrw   NDS_UITB, t0                     #/ CoDense instruction table
#endif

	/* Select PLIC mode */
#ifdef CONFIG_RISCV_VECTORED_MODE
	csrsi  NDS_MMISC_CTL, 2

	/* Enable vector feture enable address */
	lui    t0, %hi(PLIC_BASE_ADDRESS)
	addi   t0, t0, %lo(PLIC_BASE_ADDRESS)
	li     t1, 3
	sw     t1, 0(t0)
#else
	/* Disable vector mode */
	csrci  NDS_MMISC_CTL, 2

	/* Disable vector feture enable address */
	lui    t0, %hi(PLIC_BASE_ADDRESS)
	addi   t0, t0, %lo(PLIC_BASE_ADDRESS)
	li     t1, 0
	sw     t1, 0(t0)
#endif /* CONFIG_RISCV_VECTORED_MODE */

	/* Enable I/D-Cache */
	csrr   t0, NDS_MCACHE_CTL
	ori    t0, t0, 1                #/ I-Cache
	ori    t0, t0, 2                #/ D-Cache
	csrw   NDS_MCACHE_CTL, t0
	fence.i

	/* Enable misaligned access and non-blocking load */
	li     t0, (1 << 8) | (1 << 6)
	csrs   NDS_MMISC_CTL, t0

#ifdef CONFIG_TELINK_TLX_MATTER_RETENTION_LAYOUT
_RAMCODE_BLE_INIT:
	la     t1, _RAMCODE_BLE_LMA_START
	la     t2, _RAMCODE_BLE_VMA_START
	la     t3, _RAMCODE_BLE_VMA_END
_RAMCODE_BLE_INIT_BEGIN:
	bleu   t3, t2, _RAM_DLM_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAMCODE_BLE_INIT_BEGIN

_RAM_DLM_INIT:
	la     t1, _RAM_DLM_DATA_LMA_START
	la     t2, _RAM_DLM_DATA_VMA_START
	la     t3, _RAM_DLM_DATA_VMA_END
_RAM_DLM_BEGIN:
	bleu   t3, t2, _RAM_DLM_BSS_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAM_DLM_BEGIN

_RAM_DLM_BSS_INIT:
	lui    t0, 0
	la     t2, _RAM_DLM_BSS_VMA_START
	la     t3, _RAM_DLM_BSS_VMA_END
_ZERO_DLM_BSS_BEGIN:
	bleu   t3, t2, _RETENTION_DATA_INIT
	sw     t0, 0(t2)
	addi   t2, t2, 4
	j      _ZERO_DLM_BSS_BEGIN
#endif /* CONFIG_TELINK_TLX_MATTER_RETENTION_LAYOUT */

_RETENTION_DATA_INIT:
	la     t1, _RETENTION_DATA_LMA_START
	la     t2, _RETENTION_DATA_VMA_START
	la     t3, _RETENTION_DATA_VMA_END
_RETENTION_DATA_INIT_BEGIN:
	bleu   t3, t2, _START
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RETENTION_DATA_INIT_BEGIN

# _RAMCODE_RET_INIT:
# 	la     t1, _RAMCODE_RET_LMA_START
# 	la     t2, _RAMCODE_RET_VMA_START
# 	la     t3, _RAMCODE_RET_VMA_END
# _RAMCODE_RET_INIT_BEGIN:
# 	bleu   t3, t2, _RAMCODE_INIT
# 	lw     t0, 0(t1)
# 	sw     t0, 0(t2)
# 	addi   t1, t1, 4
# 	addi   t2, t2, 4
# 	j      _RAMCODE_RET_INIT_BEGIN

# _RAMCODE_INIT:
# 	la     t1, _RAMCODE_LMA_START
# 	la     t2, _RAMCODE_VMA_START
# 	la     t3, _RAMCODE_VMA_END
# _RAMCODE_INIT_BEGIN:
# 	bleu   t3, t2, _START
# 	lw     t0, 0(t1)
# 	sw     t0, 0(t2)
# 	addi   t1, t1, 4
# 	addi   t2, t2, 4
# 	j      _RAMCODE_INIT_BEGIN

_START:
	j      __start


	.option pop
