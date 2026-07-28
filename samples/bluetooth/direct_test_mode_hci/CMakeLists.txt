# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dtm_hci)

target_sources(app PRIVATE
  src/hci_uart_transport.c
  src/main.c
)
