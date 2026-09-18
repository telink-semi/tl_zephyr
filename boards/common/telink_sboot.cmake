# Copyright (c) 2026 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0
#
# Auto-detect the Telink Secure Bootloader (SBOOT) firmware and reserve the
# 20 KB (0x5000) it occupies before MCUboot when it is present.
#
# Include this file from a board's pre_dt_board.cmake so it runs before
# Kconfig. SOC_DIR / SOC_NAME / SOC_SERIES are not available at this stage
# because they are derived from CONFIG_SOC after Kconfig, so the chip is
# reverse-derived from the board directory name instead.

# BOARD_DIR is the vendor board directory, e.g. <zephyr>/boards/telink/tl323x.
get_filename_component(_telink_board_name "${BOARD_DIR}" NAME)
string(TOLOWER "${_telink_board_name}" _telink_board_lower)
string(TOUPPER "${_telink_board_lower}" _telink_soc_upper)

# The shared SOC series directory is named telink_tl5x while its only SOC is
# telink_tl5238x (TL523X). Map it explicitly so the firmware directory is not
# assembled as a bogus TL5X_FW path.
if("${_telink_soc_upper}" STREQUAL "TL5X")
  set(_telink_soc_upper "TL523X")
endif()

set(TELINK_SBOOT_DIR "${_telink_soc_upper}_FW")
set(TELINK_SBOOT_FILE
    "${ZEPHYR_BASE}/${TELINK_SBOOT_DIR}/SBOOT/Secure_Bootloader.bin")

if(EXISTS "${TELINK_SBOOT_FILE}")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/misc")
  set(TELINK_SBOOT_CONF "${CMAKE_BINARY_DIR}/misc/telink_sboot.conf")
  file(WRITE "${TELINK_SBOOT_CONF}" "CONFIG_MCUBOOT_START_OFFSET=0x5000\n")
  list(APPEND EXTRA_CONF_FILE "${TELINK_SBOOT_CONF}")
  message(STATUS "Telink SBOOT found: ${TELINK_SBOOT_FILE} "
                 "(CONFIG_MCUBOOT_START_OFFSET=0x5000)")
else()
  message(STATUS "Telink SBOOT not found: ${TELINK_SBOOT_FILE} "
                 "(CONFIG_MCUBOOT_START_OFFSET=0x0)")
endif()
