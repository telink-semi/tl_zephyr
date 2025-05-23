# Copyright (c) 2025 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

set(BASE_URL "https://doc.telink-semi.cn/Zephyr/binaries/public/w91_opus")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/zephyr")
set(OPUS_LIB_URL "${BASE_URL}/libtlka_opus_gcc_${CONFIG_TELINK_W91_OPUS_LIB_VERSION}.a")

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

# Define destination file
set(OPUS_LIB_DESTINATION "${OUTPUT_DIR}/w91_opus.a")

message(STATUS "Downloading OPUS library from ${OPUS_LIB_URL}")

file(DOWNLOAD
    "${OPUS_LIB_URL}"
    "${OPUS_LIB_DESTINATION}"
    STATUS OPUS_LIB_STATUS)

list(GET OPUS_LIB_STATUS 0 OPUS_LIB_CODE)
if(NOT OPUS_LIB_CODE EQUAL 0)
    message(FATAL_ERROR "Failed to download OPUS library: ${OPUS_LIB_STATUS}")
endif()

# --- Extract and check GCC version in the downloaded library ---
execute_process(
    COMMAND strings "${OPUS_LIB_DESTINATION}"
    COMMAND grep -i "gcc"
    OUTPUT_VARIABLE OPUS_GCC_VERSION_LINE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX MATCH "[0-9]+\\.[0-9]+" OPUS_GCC_VERSION "${OPUS_GCC_VERSION_LINE}")

if(NOT OPUS_GCC_VERSION MATCHES "^${CONFIG_TELINK_W91_OPUS_LIB_VERSION}\\.")
    message(FATAL_ERROR "GCC version mismatch: required ${CONFIG_TELINK_W91_OPUS_LIB_VERSION}.x but got ${OPUS_GCC_VERSION}")
endif()

if (CONFIG_TELINK_W91_OPUS_CODEC)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(app PRIVATE ${OPUS_LIB_DESTINATION})

    # zephyr_sources(
    #     test_opus.c
    # )
endif()