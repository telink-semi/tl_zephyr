# Telink Zephyr SDK Release Note

[![Version](https://img.shields.io/badge/Version-tl_v1.5.0--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/tl_zephyr/releases/tag/tl_v1.5.0-v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](../../../LICENSE)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)

***

- **Release Type:** Public-Release
- **Tag Version:** tl\_v1.5.0-v4.1.0

<!-- - **Branch:** dev-tlk_v4.1 -->
<!-- - **Target Commit:** 3cd5848c2a76f029097e582ccb3d27211f9cae9d -->

***

## 📖 Introduction

This release is based on the latest commit of the `dev-tlk_v4.1` branch and is the follow-up to the tl\_v1.4.1 release. It adds BLE + Thread concurrent power management, NFC subsystem support with the ST25DV tag, an OpenThread library builder sample, IEEE 802.15.4 ED scan support, and a set of SPI, IEEE 802.15.4, and interrupt-controller driver updates and fixes across the TLX and TLSR9x SoC families.

### Based on Upstream Zephyr v4.1.0

This release is based on upstream Zephyr **v4.1.0** (tag [`v4.1.0`](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)). For the full list of upstream changes, see:

- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://docs.zephyrproject.org/latest/releases/migration-guide-4.1.html) — recommended reading when upgrading from a previous Zephyr version, as upstream v4.1.0 introduced several API and Kconfig changes (Bluetooth HCI driver API rewrite, pipe API rework, removed/deprecated options, etc.).

For environment setup, see the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md).

***

## 📋 Version Information

### Zephyr SDK & Toolchain

| Component                 | Version              |
| ------------------------- | -------------------- |
| **Host OS (recommended)** | Ubuntu 24.04 LTS     |
| **Zephyr SDK Version**    | Telink Zephyr v4.1.0 |
| **Zephyr SDK**            | 0.17.0               |
| **Toolchain**             | riscv64-zephyr-elf   |

> **Note:** Only the `riscv64-zephyr-elf` toolchain (from Zephyr SDK 0.17.0) is validated for Telink SoCs. The experimental IAR compiler support introduced in upstream Zephyr v4.1.0 is **not** tested with this release. GCC-based `riscv64-zephyr-elf` is the recommended and only supported toolchain. Ubuntu 24.04 LTS is the recommended host OS.

### Telink SDK

| Property         | Value                    |
| ---------------- | ------------------------ |
| **Tag Name**     | tl\_v1.5.0-v4.1.0        |
| **Release Type** | Public-Release           |

### Chip & Hardware Versions

📦 **Chip Versions**

| Chip Family | Versions |
| ----------- | -------- |
| TL721X      | A3    |

🔧 **Hardware EVK Versions**

| Chip    | EVK Version     |
| ------- | --------------- |
| TL721X  | C1T315A20\_V1.5 |

***

## ✨ Highlights

| Category            | Details                                                                 |
| ------------------- | ----------------------------------------------------------------------- |
| **Power Management** | BLE + Thread concurrent power management with a new `ot_ble_pm` sample   |
| **New Drivers**     | Reworked NFC ST25DV tag driver and new ST25DV tag sample                 |
| **Networking**      | IEEE 802.15.4 ED scan, TL521X feature sync, RX parsing fix               |
| **Flashing**        | Auto-detect SBOOT firmware for the MCUboot start offset                  |
| **Refactoring**     | Reworked TLX PLIC driver and SoC source cleanup                          |

This release extends the tl\_v1.4.1 line with cross-platform driver updates and adds concurrent BLE + Thread low-power operation and NFC tag support across the Telink SoC families.

***

## 🆕 New Features

- ✅ BLE + Thread concurrent power management, including the new `samples/net/openthread/ot_ble_pm` sample ([#879](https://github.com/telink-semi/tl_zephyr/pull/879));
- ✅ NFC ST25DV subsystem rework with the new `samples/boards/tlsr9x/st25dv-nfc-tap` sample;
- ✅ SPI TLX driver update;
- ✅ TL521X IEEE 802.15.4 driver feature sync from TLX and B9X;
- ✅ IEEE 802.15.4 energy-detection scan (`ed_scan`) support ([#786](https://github.com/telink-semi/tl_zephyr/pull/786));
- ✅ OpenThread `lib_builder` sample project for building OpenThread libraries;
- ✅ Auto-detect SBOOT firmware for the MCUboot start offset;
- ✅ PLIC interrupt-controller driver rework with a PLIC information shell command.

***

## 🐛 Bug Fixes

| Issue | Component | Description |
| ----- | --------- | ----------- |
| [d0d8ac0](https://github.com/telink-semi/zephyr/commit/d0d8ac0d9f7) | IEEE 802.15.4 | Fix RX packet parsing sequence |
| [01b0ccd](https://github.com/telink-semi/zephyr/commit/01b0ccd152e) | Interrupt | Fix Telink PLIC driver; clean up configurations and fix ISR statistics in vectored mode |
| [1c96e97](https://github.com/telink-semi/zephyr/commit/1c96e97f529) | SoC/Build | Fix TL322X build issues |
| [3cd5848](https://github.com/telink-semi/zephyr/commit/3cd5848c2a7) | SoC/Build | Fix TL322X build issues in the IEEE 802.15.4 driver |

***

## 🔧 API & Kconfig Changes

### Telink-Specific Changes

The following Telink-specific Kconfig changes may affect existing applications when upgrading to this release:

| Change | Impact | Migration |
| ------ | ------ | --------- |
| `MCUBOOT_START_OFFSET` default changed from `0x5000` (when `MCUBOOT`) to `0x0`, with SBOOT auto-detection in `pre_dt_board.cmake` ([c6b7213](https://github.com/telink-semi/zephyr/commit/c6b72135e80)) | Normal MCUboot builds now default to offset `0`; SBOOT builds reserve `0x5000` automatically | Remove explicit `CONFIG_MCUBOOT_START_OFFSET=0x5000` overrides unless intentionally overriding the auto-detection |
| `NFC` changed from a plain `config`/`menu` to `menuconfig`; `ST25DVXXKC` now defaults to `y`, depends on `DT_HAS_ST_ST25DVXXKC_ENABLED`, and selects `I2C`; log config reworked into a `ST25DVXXK_LOG_LEVEL` choice ([6b7e149](https://github.com/telink-semi/zephyr/commit/6b7e149ec76)) | NFC/ST25DV enable behavior changes; old `ST25DVXXKC` log config is gone | Enable NFC via devicetree node status; select `ST25DVXXKC_LOG_LEVEL_*` for the desired log level |
| `PLIC_SHELL` renamed to `PLIC_TELINK_SHELL` and `PLIC_SHELL_IRQ_COUNT` renamed to `PLIC_TELINK_SHELL_IRQ_COUNT` ([01b0ccd](https://github.com/telink-semi/zephyr/commit/01b0ccd152e)) | Configurations referencing the old PLIC shell symbols break | Rename the symbols to their `PLIC_TELINK_SHELL*` equivalents |

### Upstream Inherited Changes

This release inherits all upstream Zephyr v4.1.0 API and Kconfig changes.

For the complete list, see the upstream [Zephyr v4.1.0 Release Notes — API Changes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html#api-changes) and the [Migration Guide](https://docs.zephyrproject.org/latest/releases/migration-guide-4.1.html).

***

## 📦 Updates / Dependencies

| Component                 | Repository                                                   | Commit                                                       | Notes                                                        |
| ------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **Telink BLE SDK**        | [telink-semi/tl\_ble\_sdk\_zephyr](https://github.com/telink-semi/tl_ble_sdk_zephyr) | [`eb0d120`](https://github.com/telink-semi/tl_ble_sdk_zephyr/commit/eb0d12063db5606db8bca5526ce9b54c77c45797) | Updated from `aff054a7`; fetch via `./hal_v2/fetch_sdk.sh` (hal\_v2) or `west blobs fetch hal_telink` (hal\_v1) |
| **Telink HAL Zephyr**     | [telink-semi/hal\_telink](https://github.com/telink-semi/hal_telink) | [`81ded24`](https://github.com/telink-semi/hal_telink/commit/81ded2409cb3d1461818bf3899f0c76d57f4a4d0) | Updated from `31f167f`; contains both hal\_v1 and hal\_v2 sources |
| **MCUBoot**               | [telink-semi/tl\_mcuboot](https://github.com/telink-semi/tl_mcuboot) | [`ce0da85`](https://github.com/telink-semi/tl_mcuboot/commit/ce0da85c39c749df49b0ec62b33d2ecdea24c927) | Unchanged since tl\_v1.4.1; bootloader required for OTA/DFU    |
| **OpenThread Telink**     | [telink-semi/tl\_openthread](https://github.com/telink-semi/tl_openthread) | [`542aaab`](https://github.com/telink-semi/tl_openthread/commit/542aaab44e1308e1a8a24573dfbd413fade342ee) | Unchanged since tl\_v1.4.1; OpenThread source adapted for Telink |
| **OpenThread Telink Lib** | [telink-semi/tl\_openthread\_libs](https://github.com/telink-semi/tl_openthread_libs) | [`bb57bac`](https://github.com/telink-semi/tl_openthread_libs/commit/bb57bac0857cdf4f408894ffdd9c8469d25c66cb) | Updated from `f69c186`; pre-built OpenThread library for Telink |
| **Telink XZ (LZMA)**      | [telink-semi/tl\_xz](https://github.com/telink-semi/tl_xz) | [`831f338`](https://github.com/telink-semi/tl_xz/commit/831f338fd6784661d3bec62fd01060ee4d7d373d) | Unchanged since tl\_v1.4.1; LZMA compression library module (`modules/lib/lzma`) |
| **NFC ST25DV**            | [telink-semi/nfc\_st25dv](https://github.com/telink-semi/nfc_st25dv) | [`f7e03b0`](https://github.com/telink-semi/nfc_st25dv/commit/f7e03b0616e454fed9a02c7c612a17e20b8af616) | New in this release; NFC subsystem module |
| **Telink ST25DV**         | [telink-semi/tl\_st25dv](https://github.com/telink-semi/tl_st25dv) | [`1b6e0c4`](https://github.com/telink-semi/tl_st25dv/commit/1b6e0c496b7a23392dcc48d8bce2d8955426dde5) | New in this release; Telink ST25DV tag module |

### Dependency Changes Since tl\_v1.4.1

- `telink_ble_sdk`: `aff054a7` → `eb0d120`
- `hal_telink`: `31f167f` → `81ded24`
- `tl_openthread_libs`: `f69c186` → `bb57bac`
- Added `nfc_st25dv` (`f7e03b0`) and `telink_st25dv` (`1b6e0c4`)
- `tl_mcuboot`, `tl_openthread`, and `tl_xz` remain unchanged from tl\_v1.4.1.

***

## ⚠️ Known Issues and Limitations

- No new known issues have been reported for the platforms added or updated in this release.

***

## 📌 Important Notes

- Use `west update` to automatically pull `telink_ble_sdk` along with other modules.
- Alternatively, manually fetch `telink_ble_sdk` by running `./hal_v2/fetch_sdk.sh {REPO_URL} {COMMIT_HASH}` inside `modules/hal/telink/` to pull or update the BLE stack to the version pinned by this release, where `{COMMIT_HASH}` is the `telink_ble_sdk` revision in `west.yml`. For this release, that is: `./hal_v2/fetch_sdk.sh https://github.com/telink-semi/tl_ble_sdk_zephyr.git eb0d12063db5606db8bca5526ce9b54c77c45797`
- For full environment setup instructions, refer to the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md) or the [Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/).

***

## 📊 Resource Usage (Code Size)

This section shows the RAM and ROM usage for various Zephyr samples on Telink platforms.

**Build environment:**

| Property         | Value                                                                              |
| ---------------- | ---------------------------------------------------------------------------------- |
| Zephyr SDK       | 0.17.0                                                                             |
| Toolchain        | `riscv64-zephyr-elf`                                                               |
| Extra CMake flag | `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y`                                           |
| Debug logging    | Enabled (per sample default)                                                       |
| Reproduce        | `west build -p auto -b <board> <sample> -- -DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` |

> **Note:** The numbers below are from CI builds with the configuration above.

### Supported Boards

| Board    | Chip Family |
| -------- | ----------- |
| tl7218x  | TL721X      |

***

### Zephyr Samples Support Matrix

The per-board sample support matrix is auto-generated by [`build_and_update_notes.py`](build_and_update_notes.py) from CI build logs.

> ✅ = Supported and Tested &nbsp;&nbsp; 🟡 = Supported but Untested
> &nbsp;&nbsp; (builds successfully, not functionally validated)
> &nbsp;&nbsp; · = Untested (not built or not applicable)

_TODO: run `build_and_update_notes.py` after CI builds to populate the sample support matrix._

***

### TL721X (tl7218x)

📈 **Resource Usage Details**

| Sample                               | RAMILM                      | ROM                       | RAM                         |
| ------------------------------------ | --------------------------- | ------------------------- | --------------------------- |
| **bootloader/mcuboot/boot/zephyr**    | 101952 B (38.89% of 256 KB) | 63118 B (6.02% of 1 MB)   | 14356 B (5.48% of 256 KB)   |
| **samples/basic/blinky**              | 23952 B (9.14% of 256 KB)   | 31366 B (2.99% of 1 MB)   | 1032 B (0.39% of 256 KB)    |
| **samples/basic/blinky\_pwm**         | 23952 B (9.14% of 256 KB)   | 40698 B (3.88% of 1 MB)   | 1112 B (0.42% of 256 KB)    |
| **samples/basic/button**              | 23952 B (9.14% of 256 KB)   | 32006 B (3.05% of 1 MB)   | 1048 B (0.40% of 256 KB)    |
| **samples/basic/fade\_led**           | 26000 B (9.92% of 256 KB)   | 45818 B (4.37% of 1 MB)   | 2464 B (0.94% of 256 KB)    |
| **samples/bluetooth/peripheral\_ht**  | 53792 B (20.52% of 256 KB)  | 207712 B (19.81% of 1 MB) | 10188 B (3.89% of 256 KB)   |
| **samples/crypto/mbedtls**            | 29072 B (11.09% of 256 KB)  | 68790 B (6.56% of 1 MB)   | 3112 B (1.19% of 256 KB)    |
| **samples/drivers/adc/adc\_dt**       | 25344 B (9.67% of 256 KB)   | 41010 B (3.91% of 1 MB)   | 1660 B (0.63% of 256 KB)    |
| **samples/drivers/watchdog**          | 23952 B (9.14% of 256 KB)   | 39718 B (3.79% of 1 MB)   | 1104 B (0.42% of 256 KB)    |
| **samples/hello\_world**              | 23952 B (9.14% of 256 KB)   | 30842 B (2.94% of 1 MB)   | 1032 B (0.39% of 256 KB)    |
| **samples/net/openthread/cli**        | 57392 B (21.89% of 256 KB)  | 398624 B (38.02% of 1 MB) | 47848 B (18.25% of 256 KB)  |
| **samples/net/openthread/coprocessor**| 57232 B (21.83% of 256 KB)  | 139902 B (13.34% of 1 MB) | 13056 B (4.98% of 256 KB)   |
| **samples/net/sockets/echo\_client**  | 36444 B (13.90% of 256 KB)  | 262728 B (25.06% of 1 MB) | 36468 B (13.91% of 256 KB)  |
| **samples/sensor/sht3xd**             | 23952 B (9.14% of 256 KB)   | 44442 B (4.24% of 1 MB)   | 1064 B (0.41% of 256 KB)    |
| **tests/drivers/flash/common**        | 29072 B (11.09% of 256 KB)  | 54350 B (5.18% of 1 MB)   | 1928 B (0.74% of 256 KB)    |
| **tests/drivers/hwinfo/api**          | 26240 B (10.01% of 256 KB)  | 49452 B (4.72% of 1 MB)   | 1336 B (0.51% of 256 KB)    |

***

### 📝 Additional Notes

- **Memory Regions:** May vary between chip variants; check individual board configurations
- **Full CI Data:** For complete resource usage information across all samples (including Bluetooth, OpenThread, and MCUBoot), refer to CI build artifacts
- **Production Optimizations:** For production builds, disable debug logging and enable appropriate optimizations to reduce RAM/ROM usage
- **Bluetooth & OpenThread:** For Bluetooth LE and OpenThread-specific resource usage, see the respective CI workflow files in `.github/workflows/`
- **Build Config:** All builds use `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` as in the CI pipelines

***

## 🔗 Resources

Made by Telink Semiconductor

- [Website](https://www.telink-semi.com/)
- [Forum](https://forum.telink-semi.cn/)
- [Documentation](https://doc.telink-semi.cn/)
- [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md)
- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html)
