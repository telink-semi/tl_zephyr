# Copyright (c) 2025 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

set(BASE_URL "https://doc.telink-semi.cn/Zephyr/binaries/public/w91_opus")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/zephyr")

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

set(OPUS_LIB_URL "${BASE_URL}/libtlka_opus.a")

# Define destination file
set(OPUS_LIB_DESTINATION "${OUTPUT_DIR}/w91_opus.a")

message(STATUS "Downloading OPUS library from ${OPUS_LIB_URL}")

file(DOWNLOAD
    "${OPUS_LIB_URL}"
    "${OPUS_LIB_DESTINATION}"
    SHOW_PROGRESS
    STATUS OPUS_LIB_STATUS)

list(GET OPUS_LIB_STATUS 0 OPUS_LIB_CODE)
if(NOT OPUS_LIB_CODE EQUAL 0)
    message(FATAL_ERROR "Failed to download OPUS library: ${OPUS_LIB_STATUS}")
endif()

if (CONFIG_TELINK_W91_OPUS_CODEC)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(app PRIVATE ${OPUS_LIB_DESTINATION})

    # zephyr_sources(
    #     test_opus.c
    # )
endif()