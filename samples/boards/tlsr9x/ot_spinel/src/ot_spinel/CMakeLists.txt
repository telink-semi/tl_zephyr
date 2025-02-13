# Copyright (c) 2025 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

set(OT_SPINEL_LIB "openthread_spinel")

zephyr_library_named("${OT_SPINEL_LIB}")

zephyr_library_sources(
	hdlc_interface.cpp
	spinel_manager.cpp
	spinel_radio_interface.cpp
)

target_include_directories("${OT_SPINEL_LIB}" PUBLIC
	"${ZEPHYR_OPENTHREAD_MODULE_DIR}/include"
	"${ZEPHYR_OPENTHREAD_MODULE_DIR}/src"
	"${ZEPHYR_OPENTHREAD_MODULE_DIR}/src/core"
)

zephyr_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

zephyr_link_libraries(
	"${OT_SPINEL_LIB}"
	openthread-hdlc
	openthread-radio-spinel
	openthread-radio
	openthread-spinel-rcp
	openthread-platform
)

# Add openthread defines
target_link_libraries("${OT_SPINEL_LIB}" PRIVATE
	ot-config
)

# Other openthread options
add_definitions(
    -DOPENTHREAD_CONFIG_LOG_LEVEL=${CONFIG_OPENTHREAD_LOG_LEVEL}
    -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE="openthread-core-zephyr-config.h"
)
