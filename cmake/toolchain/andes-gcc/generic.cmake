# Copyright (c) 2025 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if((NOT (DEFINED ANDES_GCC_PATH)) AND (DEFINED ENV{ANDES_GCC_PATH}))
  set(ANDES_GCC_PATH $ENV{ANDES_GCC_PATH})
endif()

set(ANDES_GCC_PATH ${ANDES_GCC_PATH} CACHE PATH "")
assert(ANDES_GCC_PATH "ANDES_GCC_PATH is not set")

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

message(STATUS "Found toolchain: andes-gcc (${ANDES_GCC_PATH})")

set(TOOLCHAIN_HOME ${ANDES_GCC_PATH})
set(CROSS_COMPILE ${ANDES_GCC_PATH}/bin/riscv32-elf-)

# Andes GCC toolchain ships target-specific headers (e.g. nds_math_types.h)
# in riscv32-elf/include/. Zephyr uses -nostdinc, so add it explicitly.
list(APPEND TOOLCHAIN_C_FLAGS -isystem ${ANDES_GCC_PATH}/riscv32-elf/include)

# Stub sys/_pthreadtypes.h for Newlib compatibility (CONFIG_NEWLIB_LIBC).
# The Andes toolchain libc does not include pthreads support.
set(NEWLIB_MATH_DIR ${ZEPHYR_BASE}/../modules/hal/telink/hal_v2/wrapper/controller/tlx/lib)
list(APPEND TOOLCHAIN_C_FLAGS -isystem ${NEWLIB_MATH_DIR})

# Andes GCC DSP library is required for riscv_dsp_* functions used by
# precompiled Telink BLE libraries (e.g. libcs_tlk1.a).
list(APPEND TOOLCHAIN_LD_FLAGS -L${ANDES_GCC_PATH}/riscv32-elf/lib/mext-dsp -ldsp)