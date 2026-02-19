# Copyright (c) 2024 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TELINK_W91_2_WIRE_SPI_ENABLE)
	message(WARNING "2-wire SPI interface enabled!")
endif()

zephyr_include_directories(.)

zephyr_sources(
	start.S
	soc_irq.S
	soc.c
	halt.c
	sys_clock.c
	sys_heap_alloc_wrap.c
	cache.c
)

zephyr_sources_ifdef(CONFIG_BT
	bluetooth/bt_mac.c
)

zephyr_sources_ifdef(CONFIG_TELINK_W91_DEBUG_ENABLE
	debug.c
)

zephyr_sources_ifdef(CONFIG_MCUBOOT_ACTION_HOOKS
	mcuboot_status_change.c
)

zephyr_sources_ifdef(CONFIG_PM
	power.c
	pm_cpu.S
)

# Force using BFD-LD
zephyr_ld_options(-fuse-ld=bfd)

# Set compile options
zephyr_compile_options_ifdef(CONFIG_ANDES_HWDSP -mext-dsp)
zephyr_compile_options_ifndef(CONFIG_RISCV_GP -mno-relax)
zephyr_linker_sources(ROM_START SORT_KEY 0x0 init.ld)
zephyr_linker_sources_ifdef(CONFIG_ANDES_CODENSE RODATA SORT_KEY 0x0 execit.ld)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 ram_aux.ld)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x1 ilm.ld)
set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
zephyr_cc_option_ifdef(CONFIG_ANDES_CODENSE -mexecit)
zephyr_ld_option_ifdef(CONFIG_ANDES_CODENSE -Wl,--mexecit)
zephyr_ld_option_ifdef(CONFIG_ANDES_HWDSP -ldsp)

# Wrap allocator functions
zephyr_link_libraries(
	-Wl,--wrap,sys_heap_alloc
	-Wl,--wrap,sys_heap_aligned_alloc
	-Wl,--wrap,sys_heap_aligned_realloc
)

if(CONFIG_TELINK_W91_ECC_HW_ACC)
zephyr_compile_definitions(
	MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
)

zephyr_include_directories(
	${ZEPHYR_MBEDTLS_MODULE_DIR}/include
	${ZEPHYR_BASE}/modules/mbedtls/configs
)

zephyr_sources(
	crypto/mbedtls_wrapper.c
	crypto/ecp_alt_backend.c
	crypto/ecp_alt_test.c
)

zephyr_link_libraries(
	-Wl,--wrap,mbedtls_ecp_check_pubkey
	-Wl,--wrap,mbedtls_ecp_mul
	-Wl,--wrap,mbedtls_ecp_mul_restartable
	-Wl,--wrap,mbedtls_ecp_muladd
	-Wl,--wrap,mbedtls_ecp_muladd_restartable
	-Wl,--wrap,mbedtls_ecp_gen_key
	-Wl,--wrap,mbedtls_ecp_gen_keypair
	-Wl,--wrap,mbedtls_ecp_gen_keypair_base
	-Wl,--wrap,mbedtls_ecp_self_test
)
endif()

if(CONFIG_TELINK_W91_FETCH_N22_BIN)

if(CONFIG_TELINK_OPUS_REMOTE)
	set(FETCH_N22_BIN_REVISION "opus_${CONFIG_TELINK_W91_FETCH_N22_BIN_REVISION}")
else()
	set(FETCH_N22_BIN_REVISION "${CONFIG_TELINK_W91_FETCH_N22_BIN_REVISION}")
endif()

	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
		${ZEPHYR_BASE}/scripts/utils/telink_w91_post_build ${ZEPHYR_BINARY_DIR} ${FETCH_N22_BIN_REVISION}
		$<IF:$<BOOL:${CONFIG_TELINK_W91_N22_MATTER_OTA_LAYOUT}>,1,0>
	)
endif()
