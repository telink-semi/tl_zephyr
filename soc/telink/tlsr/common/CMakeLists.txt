# Copyright (c) 2025 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

# This component requires from SOC UART with HW flow control
if(CONFIG_TELINK_OT_RCP)

zephyr_sources(
	ot_rcp/ot_rcp.c
	ot_rcp/hdlc_coder.c
	ot_rcp/spinel_drv.c
	ot_rcp/spinel.c
)

if(CONFIG_TELINK_OT_RCP_TRANSPORT_UART)

zephyr_sources(
	ot_rcp/transport/rcp_transport_uart.c
)

elseif(CONFIG_TELINK_OT_RCP_TRANSPORT_DUMMY)

zephyr_sources(
	ot_rcp/transport/rcp_transport_dummy.c
)

endif()

endif()

# This component requires from SOC Ethernet and some kind of IEEE802.15.4 radio
if(CONFIG_TELINK_OTBR)

zephyr_sources(
	otbr/otbr_icmpv6.c
	otbr/otbr_ext.c
	otbr/otbr_init.c
	otbr/otbr_srp.c
	otbr/otbr_route.c
	otbr/otbr_mdns.c
	otbr/otbr_mdns_fmt.c
)

zephyr_link_libraries(
	-Wl,--wrap,net_route_packet
)

endif()

# This component is used in dual cores SOC to run encoding/decoding on separate core
if(CONFIG_TELINK_OPUS_REMOTE)

if(CONFIG_ANDES_GCC)

message(STATUS "Downloading libtelink-opus.a … (this may take a while)")
file(DOWNLOAD
	https://doc.telink-semi.cn/Zephyr/binaries/public/w91_opus/${CONFIG_TELINK_OPUS_REMOTE_LIB}
	${CMAKE_BINARY_DIR}/zephyr/opus/libtelink-opus.a
	EXPECTED_HASH SHA256=${CONFIG_TELINK_OPUS_REMOTE_LIB_SHA256}
)
message(STATUS "Download finished")

message(STATUS "Downloading opus_api.h … (this may take a while)")
file(DOWNLOAD
	https://doc.telink-semi.cn/Zephyr/binaries/public/w91_opus/${CONFIG_TELINK_OPUS_REMOTE_LIB_HEADER}
	${CMAKE_BINARY_DIR}/zephyr/opus/opus_api.h
	EXPECTED_HASH SHA256=${CONFIG_TELINK_OPUS_REMOTE_LIB_HEADER_SHA256}
)
message(STATUS "Download finished")

zephyr_sources(
	opus/opus.c
)

target_link_libraries(app PRIVATE
	${CMAKE_BINARY_DIR}/zephyr/opus/libtelink-opus.a
)

zephyr_include_directories(
	${CMAKE_BINARY_DIR}/zephyr/opus
)

zephyr_linker_sources(ROM_SECTIONS SORT_KEY 0x0 opus/opus.ld)

else()

message(FATAL_ERROR "CONFIG_TELINK_OPUS_REMOTE feature requires Andes GCC")

endif()

endif()

if (CONFIG_TLSR_PROFILER)

zephyr_sources(
	tlsr_profiler/tlsr_profiler.c
	tlsr_profiler/tlsr_profiler_ext.c
)

endif()
