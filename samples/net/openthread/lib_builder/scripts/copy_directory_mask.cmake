# Copyright (c) 2026 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

file(GLOB_RECURSE TARGET_FILES
	RELATIVE "${SRC_DIR}"
	"${SRC_DIR}/${FILE_MASK}"
)

foreach(FILE_PATH ${TARGET_FILES})
	get_filename_component(DIR_PATH ${FILE_PATH} DIRECTORY)
	file(COPY "${SRC_DIR}/${FILE_PATH}"
		DESTINATION "${DEST_DIR}/${DIR_PATH}"
	)
endforeach()
