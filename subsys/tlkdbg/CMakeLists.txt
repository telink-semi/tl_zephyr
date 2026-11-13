# Copyright (c) 2026 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(include)

zephyr_library_sources(
	tlkdbg.c
	tlkdbg_log.c
	tlkdbg_tr_uart.c
	tlkdbg_uart_ll_tl323x.c
)

zephyr_library_sources_ifdef(CONFIG_TLKDBG_POLL_THREAD tlkdbg_poll.c)

# Build-time label hash table (type 4). The generator scans the application
# sources, writes the header into the build tree and the table next to the
# final zephyr.bin so the host can resolve it.
if(CONFIG_TLKDBG_LABEL_TABLE)
	find_package(Python3 REQUIRED COMPONENTS Interpreter)

	set(TLKDBG_LABEL_GEN_DIR ${CMAKE_BINARY_DIR}/generated/tlkdbg)
	set(TLKDBG_LABEL_HEADER  ${TLKDBG_LABEL_GEN_DIR}/tlkdbg_label_hashes.h)
	set(TLKDBG_LABEL_TABLE   ${CMAKE_BINARY_DIR}/zephyr/zephyr.labels.json)

	file(GLOB_RECURSE TLKDBG_LABEL_SOURCES CONFIGURE_DEPENDS
		${APPLICATION_SOURCE_DIR}/*.c
		${APPLICATION_SOURCE_DIR}/*.h)

	add_custom_command(
		OUTPUT ${TLKDBG_LABEL_HEADER} ${TLKDBG_LABEL_TABLE}
		COMMAND ${CMAKE_COMMAND} -E make_directory
			${TLKDBG_LABEL_GEN_DIR} ${CMAKE_BINARY_DIR}/zephyr
		COMMAND ${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/tools/tlkdbg_label_gen.py
			--root ${APPLICATION_SOURCE_DIR}
			--roots "${CONFIG_TLKDBG_LABEL_ROOTS}"
			--out-header ${TLKDBG_LABEL_HEADER}
			--out-table ${TLKDBG_LABEL_TABLE}
		DEPENDS ${TLKDBG_LABEL_SOURCES}
			${CMAKE_CURRENT_SOURCE_DIR}/tools/tlkdbg_label_gen.py
		COMMENT "Generating tlkdbg label hash table"
		VERBATIM)

	add_custom_target(tlkdbg_label_table
		DEPENDS ${TLKDBG_LABEL_HEADER} ${TLKDBG_LABEL_TABLE})

	zephyr_include_directories(${TLKDBG_LABEL_GEN_DIR})
	zephyr_compile_definitions(TLKDBG_LOG_USE_HASH_LABELS=1)

	add_dependencies(${ZEPHYR_CURRENT_LIBRARY} tlkdbg_label_table)
	if(TARGET app)
		add_dependencies(app tlkdbg_label_table)
	endif()
	if(TARGET zephyr)
		add_dependencies(zephyr tlkdbg_label_table)
	endif()
endif()

# The Telink HAL uart.c is already built into the Zephyr image by the HAL
# module; calling its APIs here pulls the needed objects in. Do not add it a
# second time (the symbols would be defined twice).
