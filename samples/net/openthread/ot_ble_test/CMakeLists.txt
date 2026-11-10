# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ot_cli)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/le_task.c)
if(CONFIG_BT_TLX_CHANNEL_SOUNDING)
    target_sources(app PRIVATE src/cs_reflector.c)

    # Board-level CS/antenna macros. Defined on the compile command line so they
    # take precedence over the #ifndef defaults in default_config.h board headers
    # and tlx_bt_cs.h. Applied via zephyr_interface so the Telink wrapper library
    # and the _zephyr_<soc> SDK library use identical values (FIFO sizing, ant_cfg).
    zephyr_compile_definitions(
        ANTENNA_SWITCHING_AUTO_EN=1
        NUM_ANT_SUPPORT=0x02
        MAX_ANT_PATHS_SUPPORT=0x04
    )
endif()
