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
