# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# set default overlay
file(GLOB telink_ot_lib_configs
	"overlays/*.conf"
)
if(NOT DEFINED OVERLAY_CONFIG)
	message(WARNING "***Not set OT Telink library overlay, default used***")
	list(GET telink_ot_lib_configs 0 OVERLAY_CONFIG_PATH)
	cmake_path(RELATIVE_PATH OVERLAY_CONFIG_PATH)
	set(OVERLAY_CONFIG "${OVERLAY_CONFIG_PATH}" CACHE STRING "")
else()
	set(OVERLAY_CONFIG_PATH "${OVERLAY_CONFIG}")
	cmake_path(ABSOLUTE_PATH OVERLAY_CONFIG_PATH)
	if(NOT EXISTS "${OVERLAY_CONFIG_PATH}")
		message(WARNING "***Invalid OT Telink library overlay, default used***")
		list(GET telink_ot_lib_configs 0 OVERLAY_CONFIG_PATH)
		cmake_path(RELATIVE_PATH OVERLAY_CONFIG_PATH)
		set(OVERLAY_CONFIG "${OVERLAY_CONFIG_PATH}" CACHE STRING "" FORCE)
	endif()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(telink_ot_lib)

message(STATUS "***OT Telink library overlay: ${OVERLAY_CONFIG}***")

# dummy to trigger build process
target_sources(app PRIVATE src/main.c)

add_custom_target(post_build ALL
	COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/post-build" ${CMAKE_BINARY_DIR} ${CMAKE_STRIP}
)

add_dependencies(post_build zephyr_final)

# deterministic build
set(ENV{SOURCE_DATE_EPOCH} "${CONFIG_TELINK_OT_LIB_BUILD_TIMESTAMP}")
string(TIMESTAMP OT_LIB_BUILD_TIME)
message(STATUS "***OT Telink library build time: ${OT_LIB_BUILD_TIME}***")

set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qcD <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> qD <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -D <TARGET>")

add_compile_options(
	-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.
	-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.
)

add_compile_options(-frandom-seed=0)

add_link_options(
	-Wl,--build-id=none
)
