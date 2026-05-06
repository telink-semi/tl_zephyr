# Copyright (c) 2024 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TELINK_TLX_USB_SWI_DISABLE)
	message(STATUS "USB SWI interface is disabled")
endif()

if (CONFIG_TELINK_TLX_2_WIRE_SPI_ENABLE)
	message(WARNING "2-wire SPI interface enabled!")
endif()

zephyr_include_directories(.)

zephyr_sources(
	start.S
	soc_irq.S
	soc.c
	halt.c
	sys_heap_alloc_wrap.c
)

zephyr_sources(
	idle.c
)

zephyr_sources_ifdef(CONFIG_PM
	power.c
)

zephyr_sources_ifdef(CONFIG_RISCV_VECTORED_MODE
	isr_table.c
	isr_vectored.S
)

zephyr_sources_ifdef(CONFIG_MCUBOOT_ACTION_HOOKS
	mcuboot_status_change.c
)

zephyr_sources_ifdef(CONFIG_SOC_SERIES_RISCV_TELINK_TL5X_RETENTION
	soc_irq0.c
)

# Force using BFD-LD
zephyr_ld_options(-fuse-ld=bfd)


# Set compile options
zephyr_linker_sources_ifdef(CONFIG_RISCV_VECTORED_MODE
	ROM_START
	SORT_KEY 0x0vectors
	${ZEPHYR_BASE}/include/zephyr/linker/irq-vector-table-section.ld
)
zephyr_compile_options_ifdef(CONFIG_ANDES_HWDSP -mext-dsp)
zephyr_compile_options_ifndef(CONFIG_RISCV_GP -mno-relax)
if(CONFIG_MCUBOOT)
zephyr_linker_sources(ROM_START SORT_KEY 0x0 init_boot.ld)
else()
zephyr_linker_sources(ROM_START SORT_KEY 0x0 init.ld)
endif()
zephyr_linker_sources_ifdef(CONFIG_ANDES_CODENSE RODATA SORT_KEY 0x0 execit.ld)
if(CONFIG_TELINK_TL5X_MATTER_RETENTION_LAYOUT)
if(CONFIG_MCUBOOT AND (CONFIG_SOC_RISCV_TELINK_TL321X OR CONFIG_SOC_RISCV_TELINK_TL323X))
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 mcu_boot_tlx.ld)
else()
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 matter_retention.ld)
endif()
endif()
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x1 ilm.ld)
set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
zephyr_cc_option_ifdef(CONFIG_ANDES_CODENSE -mexecit)
zephyr_ld_option_ifdef(CONFIG_ANDES_CODENSE -Wl,--mexecit)

if(CONFIG_TELINK_TL5X_MATTER_RETENTION_LAYOUT)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 matter_retention.ld)
endif()
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x1 ilm.ld)

# Wrap allocator functions
zephyr_link_libraries(
	-Wl,--wrap,sys_heap_alloc
	-Wl,--wrap,sys_heap_aligned_alloc
	-Wl,--wrap,sys_heap_aligned_realloc
	-Wl,--undefined=sys_heap_alloc
	-Wl,--undefined=sys_heap_aligned_alloc
	-Wl,--undefined=sys_heap_aligned_realloc
)

# Make MCUBoot early boot code injection
if (CONFIG_TELINK_TLX_MCUBOOT_STARTUP)

zephyr_sources(
	mcu_boot_startup.c
)

zephyr_link_libraries(
	-Wl,--wrap,main
)

endif()

# Make MCUBoot LZMA decompression code injection
if (CONFIG_COMPRESS_LZMA)

zephyr_sources(
	mcu_boot_lzma.c
)

zephyr_include_directories(
	${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/src
)

zephyr_link_libraries(
	-Wl,--wrap,boot_slots_compatible
)

endif()

zephyr_linker_sources(ROM_START SORT_KEY 0x1ramcode ramcode.ld)