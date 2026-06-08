# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(telink_ot_lib)

# dummy to trigger build process
target_sources(app PRIVATE src/main.c)

add_custom_target(post_build ALL
	COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/post-build" ${CMAKE_BINARY_DIR} ${CMAKE_STRIP}
)

add_dependencies(post_build zephyr_final)
