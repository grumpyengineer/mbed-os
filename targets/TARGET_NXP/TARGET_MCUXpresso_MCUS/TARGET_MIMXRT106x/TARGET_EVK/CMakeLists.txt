# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# MIMXRT1060_EVKB
add_library(mbed-mimxrt1060-evkb INTERFACE)

target_include_directories(mbed-mimxrt1060-evkb
    INTERFACE
        .
)

target_sources(mbed-mimxrt1060-evkb
    INTERFACE
		fsl_flexspi_nor_boot.c
		flash_api.c

        TARGET_1060_EVKB/xip/evkbmimxrt1060_flexspi_nor_config.c
        TARGET_1060_EVKB/xip/dcd.c
)

target_link_libraries(mbed-mimxrt1060-evkb
	INTERFACE
		mbed-mimxrt106x
)
