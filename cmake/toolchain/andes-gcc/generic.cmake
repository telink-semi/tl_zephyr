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

# Andes GCC DSP library is required for riscv_dsp_* functions used by
# precompiled Telink BLE libraries (e.g. libcs_tlk1.a).
list(APPEND TOOLCHAIN_LD_FLAGS -L${ANDES_GCC_PATH}/riscv32-elf/lib/mext-dsp -ldsp)

# Newlib hardware-optimized sinf/cosf to replace Picolibc's slow software version.
# Extracted from toolchain mext-dsp/libc.a at configure time.
# Only the specific objects needed for sinf/cosf are extracted to avoid
# conflicting with Picolibc (full libc.a contains all of Newlib).
set(NEWLIB_MATH_DIR ${ZEPHYR_BASE}/../modules/hal/telink/hal_v2/wrapper/controller/tlx/lib)
set(NEWLIB_MATH_LIB ${NEWLIB_MATH_DIR}/libnewlib_math.a)
set(NEWLIB_LIBC ${ANDES_GCC_PATH}/riscv32-elf/lib/mext-dsp/libc.a)

if(NOT EXISTS ${NEWLIB_MATH_LIB})
  message(STATUS "Extracting sinf/cosf objects from toolchain libc.a...")
  execute_process(
    COMMAND ${CMAKE_AR} x ${NEWLIB_LIBC}
      libm_machine_riscv_hardfloat_sf_sin.c.o
      libm_machine_riscv_hardfloat_sf_cos.c.o
      libm_machine_riscv_ref_rredf.c.o
    WORKING_DIRECTORY ${NEWLIB_MATH_DIR}
    RESULT_VARIABLE _extract_result
  )
  if(_extract_result EQUAL 0)
    execute_process(
      COMMAND ${CMAKE_AR} rcs ${NEWLIB_MATH_LIB}
        libm_machine_riscv_hardfloat_sf_sin.c.o
        libm_machine_riscv_hardfloat_sf_cos.c.o
        libm_machine_riscv_ref_rredf.c.o
      WORKING_DIRECTORY ${NEWLIB_MATH_DIR}
    )
    file(REMOVE
      ${NEWLIB_MATH_DIR}/libm_machine_riscv_hardfloat_sf_sin.c.o
      ${NEWLIB_MATH_DIR}/libm_machine_riscv_hardfloat_sf_cos.c.o
      ${NEWLIB_MATH_DIR}/libm_machine_riscv_ref_rredf.c.o
    )
  endif()
endif()

list(APPEND TOOLCHAIN_LD_FLAGS
  -Wl,--whole-archive
  ${NEWLIB_MATH_LIB}
  -Wl,--no-whole-archive
)