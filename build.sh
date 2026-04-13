# Copyright (c) 2025~2026, Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

# build command notes
west build -p -b tl3228x \
~/zephyrproject/zephyr/samples/bluetooth/peripheral_kmd/ \
-d ~/zephyrproject/build/build_peripheralkmd_tl3228x
