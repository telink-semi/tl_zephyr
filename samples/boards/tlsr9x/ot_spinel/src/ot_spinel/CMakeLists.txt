# Copyright (c) 2025 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

set(OT_SPINEL_LIB "openthread_spinel")

zephyr_library_named("${OT_SPINEL_LIB}")

zephyr_library_sources(
	hdlc_interface.cpp
)

target_include_directories("${OT_SPINEL_LIB}" PUBLIC
	"${ZEPHYR_OPENTHREAD_MODULE_DIR}/include"
	"${ZEPHYR_OPENTHREAD_MODULE_DIR}/src"
)

zephyr_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

zephyr_link_libraries(
	"${OT_SPINEL_LIB}"
)
