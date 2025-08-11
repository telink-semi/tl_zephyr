# Copyright (c) 2025 Nordic Semiconductor
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

endif()
