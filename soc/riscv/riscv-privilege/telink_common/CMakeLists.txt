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
