# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
add_subdirectory(${SOC_SERIES})
if (${SOC_SERIES} STREQUAL "telink_w91" OR
	${SOC_SERIES} STREQUAL "telink_b9x" OR
	${SOC_SERIES} STREQUAL "telink_tlx")
add_subdirectory(telink_common)
endif()
