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

if(CONFIG_TELINK_OT_RCP_UART)

zephyr_sources(
	ot_rcp/transport/rcp_transport_uart.c
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
