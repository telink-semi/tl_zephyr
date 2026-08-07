#!/usr/bin/env python3
#
#    Copyright (c) 2026 Telink Semiconductor Co., Ltd.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

import re
import shutil
import subprocess
from pathlib import Path

# Script directory = zephyr/doc/telink/releases/
SCRIPT_DIR = Path(__file__).parent
# Zephyr root = go up 3 levels (releases -> telink -> doc -> zephyr)
ZEPHYR_ROOT = SCRIPT_DIR.parent.parent.parent


def read_telink_version(zephyr_root: Path) -> str:
    """Read version from zephyr/TELINK_VERSION and assemble tl_vX.Y.Z[.W][extra].

    Example TELINK_VERSION file:
        VERSION_MAJOR = 1
        VERSION_MINOR = 0
        PATCHLEVEL = 1
        VERSION_TWEAK = 0
        EXTRAVERSION =
    """
    version_file = zephyr_root / "TELINK_VERSION"
    if not version_file.exists():
        raise FileNotFoundError(f"TELINK_VERSION file not found at {version_file}")

    fields = {}
    with open(version_file, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" in line:
                key, _, value = line.partition("=")
                fields[key.strip()] = value.strip()

    major = fields.get("VERSION_MAJOR", "0")
    minor = fields.get("VERSION_MINOR", "0")
    patch = fields.get("PATCHLEVEL", "0")
    tweak = fields.get("VERSION_TWEAK", "0")
    extra = fields.get("EXTRAVERSION", "")

    version = f"tl_v{major}.{minor}.{patch}"
    # Append tweak only if non-zero
    if tweak and tweak != "0":
        version += f".{tweak}"
    # Append EXTRAVERSION (e.g. -rc1, -beta, etc.)
    if extra:
        version += extra
    return version


def resolve_output_version(release_version: str | None, zephyr_root: Path, output_dir: Path) -> str:
    """Determine the output file version.

    - If release_version is explicitly specified, use it directly.
    - Otherwise, read the version from TELINK_VERSION.
    - If the resulting filename already exists, append _2, _3, ... suffix to avoid overwriting.
    """
    if release_version:
        version = release_version
    else:
        version = read_telink_version(zephyr_root)
        print(f"Auto-detected version from TELINK_VERSION: {version}")

    # Check for filename conflicts, append _2, _3, ...
    candidate = version
    suffix = 2
    while (output_dir / f"release-notes-{candidate}.md").exists():
        candidate = f"{version}_{suffix}"
        suffix += 1

    if candidate != version:
        print(f"File release-notes-{version}.md already exists, using {candidate} instead")

    return candidate


class TelinkBuildManager:
    def __init__(self, zephyr_root: Path | None = None, release_version: str | None = None):
        self.zephyr_root = zephyr_root or ZEPHYR_ROOT

        # Release note template (same dir as this script, has {{VERSION}} placeholder)
        self.template_filename = "release-notes-template.md"
        self.template_path = SCRIPT_DIR / self.template_filename

        # Determine output version (auto-detection + conflict handling)
        self.release_version = resolve_output_version(release_version, self.zephyr_root, SCRIPT_DIR)

        # Release note output (write, same directory as this script)
        self.release_note_filename = f"release-notes-{self.release_version}.md"
        self.output_path = SCRIPT_DIR / self.release_note_filename

        # Release build output root directory
        self.release_dir = self.zephyr_root / "build_for_release"
        self.release_dir.mkdir(exist_ok=True)

        # Build logs directory
        self.build_logs_dir = self.release_dir / "build_logs"
        self.build_logs_dir.mkdir(exist_ok=True)

        # Firmware output directory
        self.firmware_output_dir = self.release_dir / "firmware"
        self.firmware_output_dir.mkdir(exist_ok=True)

        # Define all boards and samples
        self.board_builds = self._get_board_builds()

        # Build results: key = (base_board, sample_key), value = True/False
        self.build_results: dict[tuple[str, str], bool] = {}

        # (base_board, sample_short_path) pairs that have been functionally tested.
        # Combinations that build successfully but are not in this set are marked
        # as "Supported but Untested" (🟡). Build failures are marked "Untested" (·).
        self.tested_samples: set[tuple[str, str]] = {
            # TL521X: all samples are Supported and Tested
            ("tl5218x", "basic/blinky"),
            ("tl5218x", "basic/button"),
            ("tl5218x", "basic/fade_led"),
            ("tl5218x", "hello_world"),
            ("tl5218x", "bluetooth/peripheral_ht"),
            ("tl5218x", "net/openthread/cli"),
            ("tl5218x", "crypto/mbedtls"),
            ("tl5218x", "drivers/spi_flash"),
            ("tl5218x", "drivers/watchdog"),
            ("tl5218x", "sensor/sht3xd"),
            ("tl5218x", "drivers/adc/adc_dt"),
            ("tl5218x", "net/sockets/echo_client"),
            # TL323X: all samples are Supported and Tested
            ("tl3238x", "basic/blinky"),
            ("tl3238x", "basic/button"),
            ("tl3238x", "basic/fade_led"),
            ("tl3238x", "hello_world"),
            ("tl3238x", "bluetooth/peripheral_ht"),
            ("tl3238x", "net/openthread/cli"),
            ("tl3238x", "crypto/mbedtls"),
            ("tl3238x", "drivers/spi_flash"),
            ("tl3238x", "drivers/watchdog"),
            ("tl3238x", "sensor/sht3xd"),
            ("tl3238x", "drivers/adc/adc_dt"),
            ("tl3238x", "net/sockets/echo_client"),
            ("tl3238x", "retention/basic"),
            # TL721X: core + validated samples
            ("tl7218x", "basic/blinky"),
            ("tl7218x", "basic/button"),
            ("tl7218x", "basic/fade_led"),
            ("tl7218x", "hello_world"),
            ("tl7218x", "bluetooth/peripheral_ht"),
            ("tl7218x", "net/openthread/cli"),
            ("tl7218x", "crypto/mbedtls"),
            ("tl7218x", "drivers/watchdog"),
            ("tl7218x", "net/sockets/echo_client"),
        }

        # Sample name mapping
        self.sample_map = {
            "blinky": "samples/basic/blinky",
            "hello_world": "samples/hello_world",
            "button": "samples/basic/button",
            "fade_led": "samples/basic/fade_led",
            "blinky_pwm": "samples/basic/blinky_pwm",
            "adc_dt": "samples/drivers/adc/adc_dt",
            "sht3xd": "samples/sensor/sht3xd",
            "mpu6050": "samples/sensor/mpu6050",
            "spi_flash": "samples/drivers/spi_flash",
            "spi_flash_at45": "samples/drivers/spi_flash_at45",
            "watchdog": "samples/drivers/watchdog",
            "uart_echo_bot": "samples/drivers/uart/echo_bot",
            "peripheral_ht": "samples/bluetooth/peripheral_ht",
            "cli": "samples/net/openthread/cli",
            "coprocessor": "samples/net/openthread/coprocessor",
            "echo_client": "samples/net/sockets/echo_client",
            "echo_server": "samples/net/sockets/echo_server",
            "retention_echo_client": "samples/net/sockets/echo_client",
            "retention_basic": "samples/retention/basic",
            "mbedtls": "samples/crypto/mbedtls",
            "console": "samples/usb/console",
            "cdc_acm": "samples/usb/cdc_acm",
            "common": "samples/common",
            "factorydata": "samples/factorydata",
            "smp_svr": "samples/smp_svr",
            "ml3m_button": "samples/ml3m_button",
            "gpio-kbd-matrix": "samples/boards/tlsr9x/gpio-kbd-matrix",
            "sock_simple_ipv4": "samples/boards/tlsr9x/sock_simple",
            "sock_simple_ipv6": "samples/boards/tlsr9x/sock_simple",
            "key_matrix": "samples/boards/tlsr9x/key_matrix",
            "key_pool": "samples/boards/tlsr9x/key_pool",
            "led_pool": "samples/boards/tlsr9x/led_pool",
            "pwm_pool": "samples/boards/tlsr9x/pwm_pool",
            "shell": "samples/subsys/shell/devmem_load/",
            "nvs": "samples/subsys/nvs",
            "adc_api": "tests/drivers/adc/adc_api",
        }

        # Board family mapping
        self.board_family = {
            "tl5218x": "TL521X",
            "tl5218x_retention": "TL521X",
            "tl3238x": "TL323X",
            "tl3238x_retention": "TL323X",
            "tl7218x": "TL721X",
            "tl7218x_retention": "TL721X",
            "tlsr9118bdk40d": "TLSR9118BDK40D",
            "tlsr9118bdk40d_v1": "TLSR9118BDK40D",
            "tlsr9518adk80d": "TLSR951X",
            "tlsr9528a": "TLSR952X",
            "tl3218x": "TL321X",
            "tl3228x": "TL322X",
        }

        # Board display order (as required)
        self.board_order = [
            ("tlsr9518adk80d", "TLSR951X"),
            ("tlsr9528a", "TLSR952X"),
            ("tl3218x", "TL321X"),
            ("tl3228x", "TL322X"),
            ("tl5218x", "TL521X"),
            ("tl3238x", "TL323X"),
            ("tl7218x", "TL721X"),
            ("tlsr9118bdk40d", "TLSR9118BDK40D"),
        ]

    def _get_board_builds(self) -> list[tuple[str, str, str, list[str]]]:
        """Define build configurations for all boards and samples."""
        builds = []

        # -------------------------- TL521X --------------------------
        builds.extend(
            [
                ("tl5218x", "blinky", "samples/basic/blinky", []),
                ("tl5218x", "hello_world", "samples/hello_world", []),
                ("tl5218x", "button", "samples/basic/button", []),
                ("tl5218x", "fade_led", "samples/basic/fade_led", []),
                ("tl5218x", "peripheral_ht", "samples/bluetooth/peripheral_ht", []),
                ("tl5218x", "cli", "samples/net/openthread/cli", []),
                (
                    "tl5218x",
                    "mbedtls",
                    "tests/crypto/mbedtls",
                    ["-DCONFIG_MBEDTLS_ECP_C=y", "-DCONFIG_MBEDTLS_ECP_ALL_ENABLED=y"],
                ),
                ("tl5218x", "spi_flash", "samples/drivers/spi_flash", []),
                ("tl5218x", "watchdog", "samples/drivers/watchdog", []),
                ("tl5218x", "sht3xd", "samples/sensor/sht3xd", []),
                ("tl5218x", "adc_dt", "samples/drivers/adc/adc_dt", []),
                (
                    "tl5218x_retention",
                    "retention_echo_client",
                    "samples/net/sockets/echo_client",
                    [
                        "-DOVERLAY_CONFIG=overlay-ot-sed.conf",
                        "-DCONFIG_OPENTHREAD_NETWORKKEY=\"09:24:01:56:04:4a:45:0b:23:22:1e:0e:3b:0d:0e:61:2f:1b:2c:24\"",
                        "-DCONFIG_PM=y",
                        "-DCONFIG_SOC_SERIES_RISCV_TELINK_TLX_NON_RETENTION_RAM_CODE=y",
                    ],
                ),
            ]
        )

        # -------------------------- TL323X --------------------------
        builds.extend(
            [
                ("tl3238x", "blinky", "samples/basic/blinky", []),
                ("tl3238x", "hello_world", "samples/hello_world", []),
                ("tl3238x", "button", "samples/basic/button", []),
                ("tl3238x", "fade_led", "samples/basic/fade_led", []),
                ("tl3238x", "console", "samples/subsys/usb/console", []),
                ("tl3238x", "peripheral_ht", "samples/bluetooth/peripheral_ht", []),
                ("tl3238x", "cli", "samples/net/openthread/cli", []),
                (
                    "tl3238x",
                    "mbedtls",
                    "tests/crypto/mbedtls",
                    ["-DCONFIG_MBEDTLS_ECP_C=y", "-DCONFIG_MBEDTLS_ECP_ALL_ENABLED=y"],
                ),
                ("tl3238x", "spi_flash", "samples/drivers/spi_flash", []),
                ("tl3238x", "watchdog", "samples/drivers/watchdog", []),
                ("tl3238x", "sht3xd", "samples/sensor/sht3xd", []),
                ("tl3238x", "adc_dt", "samples/drivers/adc/adc_dt", []),
                ("tl3238x", "retention_basic", "samples/retention/basic", []),
                (
                    "tl3238x_retention",
                    "retention_echo_client",
                    "samples/net/sockets/echo_client",
                    [
                        "-DOVERLAY_CONFIG=overlay-ot-sed.conf",
                        "-DCONFIG_OPENTHREAD_NETWORKKEY=\"09:24:01:56:04:4a:45:0b:23:22:1e:0e:3b:0d:0e:61:2f:1b:2c:24\"",
                        "-DCONFIG_PM=y",
                        "-DCONFIG_SOC_SERIES_RISCV_TELINK_TLX_NON_RETENTION_RAM_CODE=y",
                    ],
                ),
            ]
        )

        # -------------------------- TL721X --------------------------
        builds.extend(
            [
                ("tl7218x", "blinky", "samples/basic/blinky", []),
                ("tl7218x", "hello_world", "samples/hello_world", []),
                ("tl7218x", "button", "samples/basic/button", []),
                ("tl7218x", "fade_led", "samples/basic/fade_led", []),
                ("tl7218x", "peripheral_ht", "samples/bluetooth/peripheral_ht", []),
                ("tl7218x", "cli", "samples/net/openthread/cli", []),
                (
                    "tl7218x",
                    "echo_server",
                    "samples/net/sockets/echo_server",
                    [
                        "-DOVERLAY_CONFIG=overlay-ot.conf",
                        "-DCONFIG_OPENTHREAD_NETWORKKEY=\"09:24:01:56:04:4a:45:0b:23:22:1e:0e:3b:0d:0e:61:2f:1b:2c:24\"",
                    ],
                ),
                (
                    "tl7218x",
                    "echo_client",
                    "samples/net/sockets/echo_client",
                    [
                        "-DOVERLAY_CONFIG=overlay-ot-sed.conf",
                        "-DCONFIG_OPENTHREAD_NETWORKKEY=\"09:24:01:56:04:4a:45:0b:23:22:1e:0e:3b:0d:0e:61:2f:1b:2c:24\"",
                    ],
                ),
                (
                    "tl7218x",
                    "mbedtls",
                    "tests/crypto/mbedtls",
                    ["-DCONFIG_MBEDTLS_ECP_C=y", "-DCONFIG_MBEDTLS_ECP_ALL_ENABLED=y"],
                ),
                ("tl7218x", "spi_flash", "samples/drivers/spi_flash", []),
                ("tl7218x", "watchdog", "samples/drivers/watchdog", []),
                ("tl7218x", "sht3xd", "samples/sensor/sht3xd", []),
                ("tl7218x", "adc_dt", "samples/drivers/adc/adc_dt", []),
                (
                    "tl7218x_retention",
                    "retention_echo_client",
                    "samples/net/sockets/echo_client",
                    [
                        "-DOVERLAY_CONFIG=overlay-ot-sed.conf",
                        "-DCONFIG_OPENTHREAD_NETWORKKEY=\"09:24:01:56:04:4a:45:0b:23:22:1e:0e:3b:0d:0e:61:2f:1b:2c:24\"",
                        "-DCONFIG_PM=y",
                        "-DCONFIG_SOC_SERIES_RISCV_TELINK_TLX_NON_RETENTION_RAM_CODE=y",
                    ],
                ),
            ]
        )

        # -------------------------- TLSR9118BDK40D --------------------------
        builds.extend(
            [
                ("tlsr9118bdk40d", "blinky", "samples/basic/blinky", []),
                ("tlsr9118bdk40d", "button", "samples/basic/button", []),
                ("tlsr9118bdk40d", "uart_echo_bot", "samples/drivers/uart/echo_bot", []),
                ("tlsr9118bdk40d", "mpu6050", "samples/sensor/mpu6050", []),
                ("tlsr9118bdk40d", "spi_flash_at45", "samples/drivers/spi_flash_at45", []),
                ("tlsr9118bdk40d", "nvs", "samples/subsys/nvs", []),
                ("tlsr9118bdk40d", "peripheral_ht", "samples/bluetooth/peripheral_ht", []),
                ("tlsr9118bdk40d", "sock_simple_ipv4", "samples/boards/tlsr9x/sock_simple", []),
                (
                    "tlsr9118bdk40d",
                    "sock_simple_ipv6",
                    "samples/boards/tlsr9x/sock_simple",
                    ["-DCONFIG_APP_SOCKET_UDP_IPV6=y"],
                ),
                ("tlsr9118bdk40d", "key_matrix", "samples/boards/tlsr9x/key_matrix", []),
                ("tlsr9118bdk40d", "key_pool", "samples/boards/tlsr9x/key_pool", []),
                ("tlsr9118bdk40d", "led_pool", "samples/boards/tlsr9x/led_pool/", []),
                ("tlsr9118bdk40d", "pwm_pool", "samples/boards/tlsr9x/pwm_pool/", []),
                ("tlsr9118bdk40d", "shell", "samples/subsys/shell/devmem_load/", []),
                (
                    "tlsr9118bdk40d",
                    "mbedtls",
                    "tests/crypto/mbedtls",
                    ["-DCONFIG_MBEDTLS_ECP_C=y", "-DCONFIG_MBEDTLS_ECP_ALL_ENABLED=y"],
                ),
                ("tlsr9118bdk40d", "adc_api", "tests/drivers/adc/adc_api", []),
                ("tlsr9118bdk40d", "cli", "samples/net/openthread/cli", []),
                (
                    "tlsr9118bdk40d_v1",
                    "gpio-kbd-matrix",
                    "samples/boards/tlsr9x/gpio-kbd-matrix",
                    [],
                ),
            ]
        )

        # -------------------------- TLSR951X/B91 --------------------------
        builds.extend(
            [
                ("tlsr9518adk80d", "blinky", "samples/basic/blinky", []),
                ("tlsr9518adk80d", "peripheral_ht", "samples/bluetooth/peripheral_ht", []),
                ("tlsr9518adk80d", "gpio-kbd-matrix", "samples/boards/tlsr9x/gpio-kbd-matrix", []),
                ("tlsr9518adk80d", "mbedtls", "samples/crypto/mbedtls", []),
                ("tlsr9518adk80d", "cli", "samples/net/openthread/cli", []),
                ("tlsr9518adk80d", "retention_basic", "samples/retention/basic", []),
                ("tlsr9518adk80d", "smp_svr", "samples/smp_svr", []),
                ("tlsr9518adk80d", "console", "samples/usb/console", []),
            ]
        )

        # -------------------------- TLSR952X/B92 --------------------------
        builds.extend(
            [
                ("tlsr9528a", "blinky", "samples/basic/blinky", []),
                ("tlsr9528a", "peripheral_ht", "samples/bluetooth/peripheral_ht", []),
                ("tlsr9528a", "gpio-kbd-matrix", "samples/boards/tlsr9x/gpio-kbd-matrix", []),
                ("tlsr9528a", "mbedtls", "samples/crypto/mbedtls", []),
                ("tlsr9528a", "factorydata", "samples/factorydata", []),
                ("tlsr9528a", "cli", "samples/net/openthread/cli", []),
                ("tlsr9528a", "retention_basic", "samples/retention/basic", []),
                ("tlsr9528a", "smp_svr", "samples/smp_svr", []),
                ("tlsr9528a", "console", "samples/usb/console", []),
            ]
        )

        # -------------------------- TL321X --------------------------
        builds.extend(
            [
                ("tl3218x", "blinky", "samples/basic/blinky", []),
                ("tl3218x", "button", "samples/basic/button", []),
                ("tl3218x", "fade_led", "samples/basic/fade_led", []),
                ("tl3218x", "peripheral_ht", "samples/bluetooth/peripheral_ht", []),
                ("tl3218x", "gpio-kbd-matrix", "samples/boards/tlsr9x/gpio-kbd-matrix", []),
                ("tl3218x", "common", "samples/common", []),
                ("tl3218x", "mbedtls", "samples/crypto/mbedtls", []),
                ("tl3218x", "adc_dt", "samples/drivers/adc/adc_dt", []),
                ("tl3218x", "spi_flash", "samples/drivers/spi_flash", []),
                ("tl3218x", "watchdog", "samples/drivers/watchdog", []),
                ("tl3218x", "hello_world", "samples/hello_world", []),
                ("tl3218x", "ml3m_button", "samples/ml3m_button", []),
                ("tl3218x", "cli", "samples/net/openthread/cli", []),
                ("tl3218x", "coprocessor", "samples/net/openthread/coprocessor", []),
                ("tl3218x", "retention_basic", "samples/retention/basic", []),
                ("tl3218x", "sht3xd", "samples/sensor/sht3xd", []),
                ("tl3218x", "cdc_acm", "samples/usb/cdc_acm", []),
                ("tl3218x", "console", "samples/usb/console", []),
            ]
        )

        # -------------------------- TL322X --------------------------
        builds.extend(
            [
                ("tl3228x", "blinky", "samples/basic/blinky", []),
                ("tl3228x", "button", "samples/basic/button", []),
                ("tl3228x", "common", "samples/common", []),
                ("tl3228x", "adc_dt", "samples/drivers/adc/adc_dt", []),
                ("tl3228x", "watchdog", "samples/drivers/watchdog", []),
                ("tl3228x", "hello_world", "samples/hello_world", []),
                ("tl3228x", "sht3xd", "samples/sensor/sht3xd", []),
            ]
        )

        return builds

    @staticmethod
    def _sample_short_path(sample_name: str) -> str:
        """Return the display short path for a sample key.

        Examples:
            'blinky'          -> 'basic/blinky'
            'peripheral_ht'   -> 'bluetooth/peripheral_ht'
            'cli'             -> 'net/openthread/cli'
            'adc_dt'          -> 'drivers/adc/adc_dt'
            'gpio-kbd-matrix' -> 'boards/tlsr9x/gpio-kbd-matrix'
            'adc_api'         -> 'tests/drivers/adc/adc_api'
        """
        # Short-path mapping mirrors the actual Zephyr tree locations used in
        # sample_map; used for the support matrix table rows.
        short_map = {
            "blinky": "basic/blinky",
            "hello_world": "hello_world",
            "button": "basic/button",
            "fade_led": "basic/fade_led",
            "blinky_pwm": "basic/blinky_pwm",
            "adc_dt": "drivers/adc/adc_dt",
            "sht3xd": "sensor/sht3xd",
            "mpu6050": "sensor/mpu6050",
            "spi_flash": "drivers/spi_flash",
            "spi_flash_at45": "drivers/spi_flash_at45",
            "watchdog": "drivers/watchdog",
            "uart_echo_bot": "drivers/uart/echo_bot",
            "peripheral_ht": "bluetooth/peripheral_ht",
            "cli": "net/openthread/cli",
            "coprocessor": "net/openthread/coprocessor",
            "echo_client": "net/sockets/echo_client",
            "echo_server": "net/sockets/echo_server",
            "retention_echo_client": "net/sockets/echo_client",
            "retention_basic": "retention/basic",
            "mbedtls": "crypto/mbedtls",
            "console": "usb/console",
            "cdc_acm": "usb/cdc_acm",
            "common": "common",
            "factorydata": "factorydata",
            "smp_svr": "smp_svr",
            "ml3m_button": "ml3m_button",
            "gpio-kbd-matrix": "boards/tlsr9x/gpio-kbd-matrix",
            "sock_simple_ipv4": "boards/tlsr9x/sock_simple",
            "sock_simple_ipv6": "boards/tlsr9x/sock_simple",
            "key_matrix": "boards/tlsr9x/key_matrix",
            "key_pool": "boards/tlsr9x/key_pool",
            "led_pool": "boards/tlsr9x/led_pool",
            "pwm_pool": "boards/tlsr9x/pwm_pool",
            "shell": "subsys/shell/devmem_load",
            "nvs": "subsys/nvs",
            "adc_api": "tests/drivers/adc/adc_api",
        }
        return short_map.get(sample_name, sample_name)

    def collect_build_results_from_logs(self):
        """Infer build success/failure from existing build logs (--skip-build).

        Parses each build log's recorded return code and populates
        self.build_results so the support matrix can be generated even when
        the build step is skipped. Only collects results for (board, sample)
        combinations that are defined in the current board_builds configuration.
        """
        print()
        print("Collecting build results from existing logs...")

        # Build a whitelist of valid (base_board, sample_short_path) from board_builds
        valid_keys: set[tuple[str, str]] = set()
        for board, sample_name, _src, _extra in self.board_builds:
            base_board = board.replace("_retention", "").replace("_v1", "")
            short = self._sample_short_path(sample_name)
            valid_keys.add((base_board, short))

        self.build_results = {}
        for log_file in self.build_logs_dir.glob("build_*.log"):
            filename = log_file.name
            base = filename.replace("build_", "").replace(".log", "")

            board = None
            sample_part = None
            for b in sorted(self.board_family.keys(), key=lambda x: -len(x)):
                base_b = b.replace("_retention", "").replace("_v1", "")
                if base.startswith(base_b):
                    board = base_b
                    sample_part = base[len(base_b) :]
                    if sample_part.startswith("_"):
                        sample_part = sample_part[1:]
                    break

            if not board or not sample_part:
                continue

            ok = False
            try:
                with open(log_file, encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                m = re.search(r"Return code:\s*0\b", content)
                if m:
                    ok = True
            except OSError:
                pass

            short_path = self._sample_short_path(sample_part)
            # Collapse retention variants under the base board; if a sample
            # has any successful build, mark it as supported.
            key = (board, short_path)

            # Skip entries not in the current board_builds configuration
            if key not in valid_keys:
                continue

            self.build_results[key] = self.build_results.get(key, False) or ok

        total = len(self.build_results)
        ok_count = sum(1 for v in self.build_results.values() if v)
        print(f"  Found {total} entries in logs ({ok_count} successful).")

    def _generate_support_matrix(self) -> list[str]:
        """Generate the Zephyr Samples Support Matrix table."""
        # Column order matches the display order in board_order; collapse
        # variants (e.g. tl3238x_retention -> tl3238x).
        board_columns = []
        seen_families = set()
        for board_name, family_name in self.board_order:
            if family_name in seen_families:
                continue
            seen_families.add(family_name)
            board_columns.append((board_name, family_name))

        # Display name for family in column header (shorthand labels)
        family_header = {
            "TLSR951X": "B91 (TLSR951X)",
            "TLSR952X": "B92 (TLSR952X)",
            "TLSR9118BDK40D": "W91 (TLSR911X)",
        }

        # Build an ordered list of unique short-path sample names, drawn from
        # board_builds (covers everything that was attempted).
        sample_order: list[str] = []
        sample_seen: set[str] = set()
        for _board, sample_name, _src, _extra in self.board_builds:
            short = self._sample_short_path(sample_name)
            if short not in sample_seen:
                sample_seen.add(short)
                sample_order.append(short)

        # Group samples: top-level Zephyr directories come first in a
        # conventional order, boards/ and tests/ last.
        dir_priority = [
            "basic",
            "hello_world",
            "bluetooth",
            "net",
            "crypto",
            "drivers",
            "sensor",
            "subsys",
            "usb",
            "retention",
            "common",
            "factorydata",
            "smp_svr",
            "ml3m_button",
            "boards",
            "tests",
        ]

        def sort_key(s: str):
            top = s.split("/", 1)[0]
            try:
                pri = dir_priority.index(top)
            except ValueError:
                pri = len(dir_priority)
            return (pri, s)

        sample_order.sort(key=sort_key)

        lines = []
        lines.append("### Zephyr Samples Support Matrix")
        lines.append("")
        lines.append(
            "The table below summarizes build and test status for Zephyr samples across Telink"
        )
        lines.append(
            "chip families in this release. Samples are located under `samples/` in the Zephyr"
        )
        lines.append("tree (tests are under `tests/`).")
        lines.append("")
        lines.append("> ✅ = Supported and Tested &nbsp;&nbsp; 🟡 = Supported but Untested")
        lines.append("> &nbsp;&nbsp; (builds successfully, not functionally validated)")
        lines.append("> &nbsp;&nbsp; · = Untested (not built or not applicable)")
        lines.append("")

        header_cells = ["Sample"] + [family_header.get(f, f) for _b, f in board_columns]
        lines.append("| " + " | ".join(header_cells) + " |")
        lines.append("|" + "|".join(["--------"] + [":------------:"] * len(board_columns)) + "|")

        for sample in sample_order:
            row_cells = [f"**{sample}**"]
            for board_name, _family_name in board_columns:
                built_ok = self.build_results.get((board_name, sample), False)
                is_tested = (board_name, sample) in self.tested_samples
                if built_ok and is_tested:
                    mark = "✅"
                elif built_ok:
                    mark = "🟡"
                else:
                    mark = "·"
                row_cells.append(mark)
            lines.append("| " + " | ".join(row_cells) + " |")

        lines.append("")
        lines.append("#### Notes on Sample Support")
        lines.append("")
        lines.append(
            "- **Tested combinations (✅):** All TL521X and TL323X samples that build"
        )
        lines.append(
            "  successfully have been"
        )
        lines.append(
            "  functionally validated. On TL721X, the core bring-up samples (`blinky`, `button`,"
        )
        lines.append(
            "  `fade_led`, `hello_world`) plus BLE (`peripheral_ht`), Thread (`openthread/cli`),"
        )
        lines.append(
            "  crypto (`mbedtls`), networking (`echo_client`), and driver (`watchdog`) samples"
        )
        lines.append("  have been functionally validated in this release.")
        lines.append(
            "- **Supported but untested (🟡):** All other build targets listed in the table"
        )
        lines.append(
            "  compile successfully but have not been functionally validated. Use with caution."
        )
        lines.append(
            "- **Untested (·):** Combinations marked · are not built in this release, either"
        )
        lines.append("  because the sample is not applicable to that chip family or because it has")
        lines.append("  not been ported yet.")
        lines.append(
            "- **Legacy platforms (B91/B92/TL321X/TL322X/W91):** Samples marked 🟡 compile in"
        )
        lines.append(
            "  CI but functional testing in this release focused on the new TL323X platform"
        )
        lines.append(
            "  and the TL721X hal_v2 migration. Refer to earlier release notes for validated"
        )
        lines.append("  sample sets on these platforms.")
        lines.append("")
        lines.append("---")
        lines.append("")
        return lines

    def build_all(self):
        """Build all samples for all boards."""
        print("=" * 80)
        print("Building all Telink samples...")
        print("=" * 80)
        print()

        success_count = 0
        fail_count = 0

        for board, sample_name, source, extra_args in self.board_builds:
            build_dir_name = f"build_{board}_{sample_name}"
            build_dir = self.release_dir / build_dir_name

            # Remove _retention suffix for log filename and base board
            base_board_name = board.replace("_retention", "").replace("_v1", "")
            log_filename = f"build_{base_board_name}_{sample_name}.log"
            log_file = self.build_logs_dir / log_filename

            print(f"  Building {board} {sample_name}...")
            print(f"  Source: {source}")

            cmd = [
                "west",
                "build",
                "-b",
                board,
                "-d",
                str(build_dir),
                source,
                "--",
                "-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y",
            ] + extra_args

            print(f"  Command: {' '.join(cmd)}")

            # Execute build
            result = subprocess.run(
                cmd, capture_output=True, text=True, cwd=str(self.zephyr_root), check=False
            )

            # Write log
            with open(log_file, "w", encoding="utf-8") as f:
                f.write(f"Command: {' '.join(cmd)}\n\n")
                f.write(f"STDOUT:\n{result.stdout}\n\n")
                f.write(f"STDERR:\n{result.stderr}\n\n")
                f.write(f"Return code: {result.returncode}\n")

            # Copy build.log
            build_log = build_dir / "build.log"
            if build_log.exists():
                with open(log_file, "ab") as f:
                    f.write(b"\n\n" + b"=" * 80 + b"\n")
                    f.write(b"build.log:\n\n")
                    with open(build_log, "rb") as src:
                        shutil.copyfileobj(src, f)

            ok = result.returncode == 0
            # Record build result for the support matrix; _retention/_v1 board
            # variants collapse to the base board.
            short_path = self._sample_short_path(sample_name)
            self.build_results[(base_board_name, short_path)] = ok

            if ok:
                print(f"  Success! Log written to {log_file}")
                success_count += 1
            else:
                print(f"  Failed! Log written to {log_file}")
                fail_count += 1

            print()

        print("=" * 80)
        print(f"Build complete! {success_count} succeeded, {fail_count} failed.")
        print("=" * 80)

    def extract_memory_info(self) -> dict[str, dict[str, list[dict]]]:
        """Extract memory usage information from build logs."""
        data = {}

        print()
        print("=" * 80)
        print("Extracting memory usage information from logs...")
        print("=" * 80)
        print()

        for log_file in self.build_logs_dir.glob("build_*.log"):
            filename = log_file.name
            base = filename.replace("build_", "").replace(".log", "")

            # Find board
            board = None
            sample_part = None
            for b in sorted(self.board_family.keys(), key=lambda x: -len(x)):
                if base.startswith(b):
                    board = b
                    sample_part = base[len(b) :]
                    if sample_part.startswith("_"):
                        sample_part = sample_part[1:]
                    break

            if not board:
                # Try matching without _retention/_v1 suffix
                for b in sorted(self.board_family.keys(), key=lambda x: -len(x)):
                    base_b = b.replace("_retention", "").replace("_v1", "")
                    if base.startswith(base_b):
                        board = base_b
                        sample_part = base[len(base_b) :]
                        if sample_part.startswith("_"):
                            sample_part = sample_part[1:]
                        break

            if not board:
                continue

            sample_name = self.sample_map.get(sample_part, f"samples/{sample_part}")

            # Read file
            with open(log_file, encoding="utf-8", errors="ignore") as f:
                content = f.read()

            # Find memory information
            idx = content.find("Memory region")
            if idx == -1:
                continue

            # Extract memory region
            memory_end = content.find("Generating files from", idx)
            if memory_end == -1:
                memory_end = idx + 600

            memory_text = content[idx:memory_end]

            # Parse
            memory_regions = []
            lines = memory_text.split("\n")
            for line in lines[1:]:
                line = line.strip()
                if not line:
                    continue
                if "IDT_LIST" in line:
                    break

                # Parse with regex
                match = re.match(r"(\S+):\s*(\d+)\s*(\S+)\s*(\d+)\s*(\S+)\s*([\d.]+)%", line)
                if match:
                    name = match.group(1).rstrip(":")
                    used = f"{match.group(2)} {match.group(3)}"
                    size = f"{match.group(4)} {match.group(5)}"
                    percent = f"{match.group(6)}%"
                    memory_regions.append(
                        {"name": name, "used": used, "size": size, "percent": percent}
                    )
                else:
                    # Alternative format, e.g. RAM_ILM_N:         27 KB       128 KB     21.09%
                    match2 = re.match(
                        r"(\S+):\s*([\d.]+)\s*(\S+)\s*([\d.]+)\s*(\S+)\s*([\d.]+)%", line
                    )
                    if match2:
                        name = match2.group(1).rstrip(":")
                        used = f"{match2.group(2)} {match2.group(3)}"
                        size = f"{match2.group(4)} {match2.group(5)}"
                        percent = f"{match2.group(6)}%"
                        memory_regions.append(
                            {"name": name, "used": used, "size": size, "percent": percent}
                        )

            if memory_regions:
                # Find base board name (without _retention/_v1 suffix)
                base_board = board.replace("_retention", "").replace("_v1", "")
                family = None
                for b, f in self.board_family.items():
                    if b.startswith(base_board):
                        family = f
                        break

                if not family:
                    family = self.board_family.get(base_board, base_board.upper())

                if family not in data:
                    data[family] = {}
                if base_board not in data[family]:
                    data[family][base_board] = []
                data[family][base_board].append({"name": sample_name, "memory": memory_regions})

        total_samples = sum(len(b) for f in data.values() for b in f.values())
        print(f"Processed {total_samples} samples successfully!")

        return data

    def update_release_notes(self, data: dict[str, dict[str, list[dict]]]):
        """Generate release notes file from template."""
        print()
        print("=" * 80)
        print(f"Generating {self.release_note_filename} from template {self.template_filename}...")
        print("=" * 80)
        print()

        # Check if template file exists
        if not self.template_path.exists():
            print(f"Error: Template file {self.template_path} does not exist!")
            return

        # Build new Resource Usage section
        resource_usage_lines = []
        resource_usage_lines.append("## 📊 Resource Usage (Code Size)")
        resource_usage_lines.append("")
        resource_usage_lines.append(
            "This section shows the RAM and ROM usage for various Zephyr samples "
            "on Telink platforms."
        )
        resource_usage_lines.append("")

        # Build environment table
        resource_usage_lines.append("**Build environment:**")
        resource_usage_lines.append("")
        resource_usage_lines.append("| Property | Value |")
        resource_usage_lines.append("|----------|-------|")
        resource_usage_lines.append("| Zephyr SDK | 0.17.0 |")
        resource_usage_lines.append("| Toolchain | `riscv64-zephyr-elf` |")
        resource_usage_lines.append("| Optimization | Default (per sample) |")
        resource_usage_lines.append(
            "| Extra CMake flag | `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` |"
        )
        resource_usage_lines.append("| Debug logging | Enabled (per sample default) |")
        resource_usage_lines.append(
            "| Reproduce | `west build -p auto -b <board> <sample> -- "
            "-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` |"
        )
        resource_usage_lines.append("")
        resource_usage_lines.append(
            "> **Note:** The numbers below are from CI builds with the configuration above."
        )
        resource_usage_lines.append("")

        # Supported Boards table
        resource_usage_lines.append("### Supported Boards")
        resource_usage_lines.append("")
        resource_usage_lines.append("| Board | Chip Family |")
        resource_usage_lines.append("|-------|-------------|")
        resource_usage_lines.append("| tlsr9518adk80d | TLSR951X/B91 |")
        resource_usage_lines.append("| tlsr9528a | TLSR952X/B92 |")
        resource_usage_lines.append("| tl3218x | TL321X |")
        resource_usage_lines.append("| tl3228x | TL322X |")
        resource_usage_lines.append("| tl5218x | TL521X |")
        resource_usage_lines.append("| tl3238x | TL323X |")
        resource_usage_lines.append("| tl7218x | TL721X |")
        resource_usage_lines.append("| tlsr9118bdk40d | TLSR9118BDK40D |")
        resource_usage_lines.append("")
        resource_usage_lines.append("---")
        resource_usage_lines.append("")

        # Zephyr Samples Support Matrix (auto-generated from build results)
        resource_usage_lines.extend(self._generate_support_matrix())

        # Per-chip-family resource usage tables
        for board_name, family_name in self.board_order:
            if family_name not in data or board_name not in data[family_name]:
                print(f"Warning: No data for {board_name} ({family_name})")
                continue

            resource_usage_lines.append(f"### {family_name} ({board_name})")
            resource_usage_lines.append("")
            resource_usage_lines.append("📈 **Resource Usage Details**")
            resource_usage_lines.append("")

            samples = sorted(data[family_name][board_name], key=lambda x: x["name"])

            if not samples:
                continue

            # Get memory region names (use the first sample's region names as headers)
            region_names = [r["name"] for r in samples[0]["memory"]]

            # Headers
            headers = ["Sample"] + region_names
            resource_usage_lines.append("| " + " | ".join(headers) + " |")
            resource_usage_lines.append(
                "|" + "|".join(["--------"] + ["-----"] * len(region_names)) + "|"
            )

            # Data rows
            for sample in samples:
                row = [f"**{sample['name']}**"]
                region_map = {r["name"]: r for r in sample["memory"]}
                for region_name in region_names:
                    if region_name in region_map:
                        r = region_map[region_name]
                        row.append(f"{r['used']} ({r['percent']} of {r['size']})")
                    else:
                        row.append("N/A")
                resource_usage_lines.append("| " + " | ".join(row) + " |")

            resource_usage_lines.append("")
            resource_usage_lines.append("---")
            resource_usage_lines.append("")

        # Additional Notes (auto-appended so the template does not need to carry it)
        resource_usage_lines.append("### 📝 Additional Notes")
        resource_usage_lines.append("")
        resource_usage_lines.append(
            "- **Memory Regions:** May vary between chip variants; check individual "
            "board configurations"
        )
        resource_usage_lines.append(
            "- **Full CI Data:** For complete resource usage information across all "
            "samples (including Bluetooth, OpenThread, and MCUBoot), refer to the "
            "CI build artifacts"
        )
        resource_usage_lines.append(
            "- **Production Optimizations:** For production builds, disable debug "
            "logging and enable appropriate optimizations to reduce RAM/ROM usage"
        )
        resource_usage_lines.append(
            "- **Bluetooth & OpenThread:** For Bluetooth LE and OpenThread-specific "
            "resource usage, see the respective CI workflow files in "
            "`.github/workflows/`"
        )
        resource_usage_lines.append(
            "- **Build Config:** All builds use `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` "
            "as in the CI pipelines"
        )
        resource_usage_lines.append("")
        resource_usage_lines.append("---")
        resource_usage_lines.append("")

        # Read template file
        with open(self.template_path, encoding="utf-8") as f:
            template_lines = f.read().split("\n")

        # Replace {{VERSION}} placeholder in template with target version
        template_lines = [
            line.replace("{{VERSION}}", self.release_version) for line in template_lines
        ]
        print(f"Substituted version placeholder: {{{{VERSION}}}} -> {self.release_version}")

        # Splice: replace the template's Resource Usage section (from the
        # "## 📊 Resource Usage" header up to but not including the next top-level
        # section that follows it, i.e. "## 🔗 Resources") with our generated
        # content. The template only carries a one-line _TODO_ description
        # inside this section; the real content is generated here.
        new_lines = []
        i = 0
        in_resource_usage = False
        while i < len(template_lines):
            line = template_lines[i]

            # Detect start of Resource Usage section
            if not in_resource_usage and line.startswith("## ") and "Resource Usage" in line:
                in_resource_usage = True
                # Emit generated content instead of the placeholder header/body
                new_lines.extend(resource_usage_lines)
                i += 1
                # Skip lines until we hit the next ## section at the same level
                while i < len(template_lines):
                    if (
                        template_lines[i].startswith("## ")
                        and "Resource Usage" not in template_lines[i]
                    ):
                        break
                    i += 1
                in_resource_usage = False
                continue

            new_lines.append(line)
            i += 1

        # Write output file
        with open(self.output_path, "w", encoding="utf-8") as f:
            f.write("\n".join(new_lines))

        print(f"Generated {self.release_note_filename} successfully at {self.output_path}")

    def collect_firmware(self):
        """Collect firmware files for all samples of all boards."""
        print()
        print("=" * 80)
        print("Collecting firmware files...")
        print("=" * 80)
        print()

        # Firmware file types to collect
        firmware_files = ["zephyr.bin", "zephyr.elf", "zephyr.hex", "zephyr.dts", ".config"]

        # Organize firmware by board
        board_firmware = {}

        for board, sample_name, source, _extra_args in self.board_builds:
            build_dir_name = f"build_{board}_{sample_name}"
            build_dir = self.release_dir / build_dir_name
            zephyr_dir = build_dir / "zephyr"

            if not zephyr_dir.exists():
                continue

            # Get base board name
            base_board_name = board.replace("_retention", "").replace("_v1", "")

            if base_board_name not in board_firmware:
                board_firmware[base_board_name] = []

            # Create board sample directory
            sample_firmware_dir = self.firmware_output_dir / base_board_name / sample_name
            sample_firmware_dir.mkdir(parents=True, exist_ok=True)

            # Copy firmware files
            copied_files = []
            for fw_file in firmware_files:
                src_file = zephyr_dir / fw_file
                if src_file.exists():
                    dst_file = sample_firmware_dir / fw_file
                    shutil.copy2(src_file, dst_file)
                    copied_files.append(fw_file)

            # Record successful firmware collection
            if copied_files:
                board_firmware[base_board_name].append(
                    {
                        "sample_name": sample_name,
                        "source": source,
                        "directory": sample_firmware_dir,
                        "files": copied_files,
                    }
                )
                print(f"  Collected: {base_board_name}/{sample_name} - {len(copied_files)} files")

        return board_firmware

    def create_firmware_archives(self, board_firmware: dict[str, list[dict]]):
        """Create a separate archive for each board."""
        print()
        print("=" * 80)
        print("Creating firmware archives...")
        print("=" * 80)
        print()

        archives_created = []

        for board_name, samples in board_firmware.items():
            if not samples:
                continue

            # Create archive filename
            archive_path = self.firmware_output_dir / f"{board_name}_firmware.zip"

            # Delete existing archive
            if archive_path.exists():
                archive_path.unlink()

            # Create archive
            print(f"  Creating archive for {board_name}...")

            import zipfile

            with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
                for sample in samples:
                    sample_dir = sample["directory"]
                    for file_name in sample["files"]:
                        file_path = sample_dir / file_name
                        arcname = f"{board_name}/{sample['sample_name']}/{file_name}"
                        zipf.write(file_path, arcname)

            archives_created.append(archive_path)
            print(f"  Created: {archive_path}")

        return archives_created

    def run(self, build: bool = True, collect_firmware: bool = True):
        """Run the complete workflow."""
        if build:
            self.build_all()
        else:
            self.collect_build_results_from_logs()

        data = self.extract_memory_info()
        self.update_release_notes(data)

        # Collect and archive firmware
        if collect_firmware:
            board_firmware = self.collect_firmware()
            self.create_firmware_archives(board_firmware)

        print()
        print("=" * 80)
        print("All tasks complete!")
        print("=" * 80)


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description="Build Telink samples and generate release notes from a template.",
        allow_abbrev=False,
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip building samples, just update release notes from existing logs",
    )
    parser.add_argument(
        "--skip-firmware", action="store_true", help="Skip collecting and archiving firmware"
    )
    parser.add_argument(
        "--release-version",
        default=None,
        help="Target release version for the output file. "
        "If not specified, the version is auto-detected from zephyr/TELINK_VERSION. "
        "If the resulting file already exists, a _2, _3, ... suffix is appended. "
        "The output file is release-notes-<release-version>.md in the script directory. "
        "The template file release-notes-template.md is always used as input.",
    )
    args = parser.parse_args()

    manager = TelinkBuildManager(release_version=args.release_version)

    # Modify run method to support skipping firmware collection
    manager.run(build=not args.skip_build, collect_firmware=not args.skip_firmware)


if __name__ == "__main__":
    main()
