# Telink Zephyr SDK Release Note

[![Version](https://img.shields.io/badge/Version-tl__v1.6.0--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/tl_zephyr/releases/tag/tl_v1.6.0-v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](../../../LICENSE)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)

***

- **Release Type:** Public-Release
- **Tag Version:** tl\_v1.6.0-v4.1.0
- **Branch:** dev-tlk_v4.1


***

## 📖 Introduction

This release is based on the latest commit of the `dev-tlk_v4.1` branch and is
the follow-up to the tl\_v1.5.0 release. It provides always-on (non-PM) device
support for the TL5218X platform, along with TL523X BLE function support, BLE
central role support, low-battery-detect demos, and a set of GPIO, SoC, and
SWS-related fixes across the TLX SoC families.

### Based on Upstream Zephyr v4.1.0

This release is based on upstream Zephyr **v4.1.0** (tag [`v4.1.0`](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)). For the full list of upstream changes, see:

- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://github.com/zephyrproject-rtos/zephyr/blob/main/doc/releases/migration-guide-4.1.rst) — recommended reading when upgrading from a previous Zephyr version, as upstream v4.1.0 introduced several API and Kconfig changes (Bluetooth HCI driver API rewrite, pipe API rework, removed/deprecated options, etc.).

For environment setup, see the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md).

***

## 🆕 Changes in tl_v1.6.0

This release brings the following changes on top of tl\_v1.5.0:

- **TL5218X always-on support:** Validated Zephyr samples for the TL5218X
  platform in always-on (non-PM) mode
- **New SoC support:** TL523X BLE functions support ([#892](https://github.com/telink-semi/tl_zephyr/pull/892))
- **BLE:** Add central role support ([#893](https://github.com/telink-semi/tl_zephyr/pull/893))
- **Demos:** Add low-battery-detect demos ([#895](https://github.com/telink-semi/tl_zephyr/pull/895))
- **Bug fixes:**
  - Fix GPIO interrupt enable placement in Telink GPIO drivers ([#889](https://github.com/telink-semi/tl_zephyr/pull/889))
  - Update peak BTR for TL521X/TL323X to improve RF performance ([#891](https://github.com/telink-semi/tl_zephyr/pull/891))
  - Gate `gpio_shutdown` with `CONFIG_PM` and note SWS pull-up ([#896](https://github.com/telink-semi/tl_zephyr/pull/896))
  - Drop redundant SWS pull-up from restore hooks ([#897](https://github.com/telink-semi/tl_zephyr/pull/897))
- **Dependencies:** Update `hal_telink` to `3cc9054` and `telink_ble_sdk` to `8c43a9d`

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
| **Tag Name**     | tl\_v1.6.0-v4.1.0        |
| **Release Type** | Public-Release           |

### Chip & Hardware Versions

📦 **Chip Versions**

| Chip Family | Versions |
| ----------- | -------- |
| TL521X      | A0       |

🔧 **Hardware EVK Versions**

| Chip   | EVK Version                   |
| ------ | ----------------------------- |
| TL521X | C1T416A20_V1.0/C1T417A20_V1.0 |

***

## ✨ Highlights

| Category            | Details                                                                 |
| ------------------- | ----------------------------------------------------------------------- |
| **TL5218X Support** | Always-on (non-PM) device support with validated Zephyr samples         |
| **New SoC Support** | TL523X BLE functions support                                            |
| **BLE**             | Central role support; peak BTR update for TL521X/TL323X                 |
| **Demos**           | Low-battery-detect demos                                                |
| **Bug Fixes**       | GPIO interrupt fix; SWS pull-up cleanup; `gpio_shutdown` PM gating      |

This release extends the tl\_v1.5.0 line with TL5218X always-on support, TL523X
BLE support, BLE central role, and a set of SoC and GPIO fixes.

***

## 🆕 New Features

- ✅ TL5218X always-on (non-PM) device support with validated Zephyr samples;
- ✅ TL523X BLE functions support ([#892](https://github.com/telink-semi/tl_zephyr/pull/892));
- ✅ BLE central role support ([#893](https://github.com/telink-semi/tl_zephyr/pull/893));
- ✅ Low-battery-detect demos ([#895](https://github.com/telink-semi/tl_zephyr/pull/895)).

***

## 🐛 Bug Fixes

| Issue | Component | Description |
| ----- | --------- | ----------- |
| [#889](https://github.com/telink-semi/tl_zephyr/pull/889) | GPIO | Fix interrupt enable placement in Telink GPIO drivers |
| [#891](https://github.com/telink-semi/tl_zephyr/pull/891) | SoC/RF | Update peak BTR for TL521X/TL323X to improve RF performance |
| [#896](https://github.com/telink-semi/tl_zephyr/pull/896) | SoC/PM | Gate `gpio_shutdown` with `CONFIG_PM` and note SWS pull-up |
| [#897](https://github.com/telink-semi/tl_zephyr/pull/897) | SoC | Drop redundant SWS pull-up from restore hooks |

***

## 🔧 API & Kconfig Changes

### Telink-Specific Changes

No new Telink-specific API or Kconfig changes in this release.

### Upstream Inherited Changes

This release inherits all upstream Zephyr v4.1.0 API and Kconfig changes.

For the complete list, see the upstream [Zephyr v4.1.0 Release Notes — API Changes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html#api-changes) and the [Migration Guide](https://github.com/zephyrproject-rtos/zephyr/blob/main/doc/releases/migration-guide-4.1.rst).

***

## 📦 Updates / Dependencies

| Component                 | Repository                                                   | Commit                                                       | Notes                                                        |
| ------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **Telink BLE SDK**        | [telink-semi/tl\_ble\_sdk\_zephyr](https://github.com/telink-semi/tl_ble_sdk_zephyr) | [`8c43a9d`](https://github.com/telink-semi/tl_ble_sdk_zephyr/commit/8c43a9d44b146ce58e23ffcb29a483aa06b29383) | Updated from `eb0d120`; peak BTR update for TL521X/TL323X; fetch via `./hal_v2/fetch_sdk.sh` (hal\_v2) or `west blobs fetch hal_telink` (hal\_v1) |
| **Telink HAL Zephyr**     | [telink-semi/hal\_telink](https://github.com/telink-semi/hal_telink) | [`3cc9054`](https://github.com/telink-semi/hal_telink/commit/3cc9054edd5edcaa1a8b5870099af81433afaf58) | Updated from `81ded24`; add peripheral\_ht (#212) and central (#213) support |
| **MCUBoot**               | [telink-semi/tl\_mcuboot](https://github.com/telink-semi/tl_mcuboot) | [`ce0da85`](https://github.com/telink-semi/tl_mcuboot/commit/ce0da85c39c749df49b0ec62b33d2ecdea24c927) | Unchanged since tl\_v1.4.1; bootloader required for OTA/DFU    |
| **OpenThread Telink**     | [telink-semi/tl\_openthread](https://github.com/telink-semi/tl_openthread) | [`542aaab`](https://github.com/telink-semi/tl_openthread/commit/542aaab44e1308e1a8a24573dfbd413fade342ee) | Unchanged since tl\_v1.4.1; OpenThread source adapted for Telink |
| **OpenThread Telink Lib** | [telink-semi/tl\_openthread\_libs](https://github.com/telink-semi/tl_openthread_libs) | [`bb57bac`](https://github.com/telink-semi/tl_openthread_libs/commit/bb57bac0857cdf4f408894ffdd9c8469d25c66cb) | Unchanged since tl\_v1.5.0; pre-built OpenThread library for Telink |
| **Telink XZ (LZMA)**      | [telink-semi/tl\_xz](https://github.com/telink-semi/tl_xz) | [`831f338`](https://github.com/telink-semi/tl_xz/commit/831f338fd6784661d3bec62fd01060ee4d7d373d) | Unchanged since tl\_v1.4.1; LZMA compression library module (`modules/lib/lzma`) |
| **NFC ST25DV**            | [telink-semi/nfc\_st25dv](https://github.com/telink-semi/nfc_st25dv) | [`f7e03b0`](https://github.com/telink-semi/nfc_st25dv/commit/f7e03b0616e454fed9a02c7c612a17e20b8af616) | Unchanged since tl\_v1.5.0; NFC subsystem module |
| **Telink ST25DV**         | [telink-semi/tl\_st25dv](https://github.com/telink-semi/tl_st25dv) | [`1b6e0c4`](https://github.com/telink-semi/tl_st25dv/commit/1b6e0c496b7a23392dcc48d8bce2d8955426dde5) | Unchanged since tl\_v1.5.0; Telink ST25DV tag module |

### Dependency Changes Since tl\_v1.5.0

- `hal_telink`: `81ded24` → `3cc9054` (tag: `tl_v1.6.0-v4.0.4.8`)
- `telink_ble_sdk`: `eb0d120` → `8c43a9d` (tag: `tl_v1.6.0-v4.0.4.8`)
- `tl_mcuboot`, `tl_openthread`, `tl_openthread_libs`, `tl_xz`, `nfc_st25dv`, and `telink_st25dv` remain unchanged from tl\_v1.5.0.

***

## ⚠️ Known Issues and Limitations

- **TL521X HAL:** TL521X uses **hal_v2** in this release. The BLE SDK is fetched automatically via `west update`. Use `fetch_sdk.sh` only to pin a non-default commit.
- **Toolchain:** Only `riscv64-zephyr-elf` is validated for Telink SoCs. Other toolchains (e.g. experimental IAR) are not tested.
- **spi_flash sample:** The `samples/drivers/spi_flash` sample now includes a `tl5218x.overlay` to enable the external SPI NOR flash (P25Q16) on the GSPI bus.

***

## 📌 Important Notes

- Use `west update` to automatically pull `telink_ble_sdk` along with other modules.
- To manually fetch or pin a specific `telink_ble_sdk` commit, run `./hal_v2/fetch_sdk.sh {REPO_URL} {COMMIT_HASH}` inside `modules/hal/telink/`. For this release: `./hal_v2/fetch_sdk.sh https://github.com/telink-semi/tl_ble_sdk_zephyr.git 8c43a9d44b146ce58e23ffcb29a483aa06b29383`
- For full environment setup instructions, refer to the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md) or the [Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/).

***

## 📊 Resource Usage (Code Size)

This section shows the RAM and ROM usage for Zephyr samples on the TL5218X
platform (always-on, non-PM mode).

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
| tl5218x  | TL521X      |

***

### Zephyr Samples Support Matrix

> ✅ = Supported and Tested &nbsp;&nbsp; 🟡 = Supported but Untested
> &nbsp;&nbsp; (builds successfully, not functionally validated)
> &nbsp;&nbsp; · = Untested (not built or not applicable)

| Sample                         | TL521X |
| ------------------------------ | :----: |
| **basic/blinky**               |   ✅    |
| **basic/blinky\_pwm**          |   🟡    |
| **basic/button**               |   ✅    |
| **basic/fade\_led**            |   ✅    |
| **hello\_world**               |   ✅    |
| **bluetooth/peripheral\_ht**   |   ✅    |
| **net/openthread/cli**         |   ✅    |
| **net/openthread/coprocessor** |   🟡    |
| **crypto/mbedtls**             |   ✅    |
| **drivers/adc/adc\_dt**        |   ✅    |
| **drivers/spi\_flash**         |   🟡    |
| **drivers/watchdog**           |   ✅    |
| **sensor/sht3xd**              |   ✅    |
| **tests/drivers/flash/common** |   🟡    |
| **tests/drivers/hwinfo/api**   |   🟡    |
| **bootloader/mcuboot**         |   🟡    |

> **Notes on Sample Support**
>
> - **Tested combinations (✅):** All samples listed as ✅ have been built and
>   functionally validated on TL521X.
> - **Supported but untested (🟡):** some samples can compile successfully but
>   have not been functionally validated.
> - **Untested (·):** Combinations not listed are not built or not applicable.
***

### TL521X (tl5218x)

📈 **Resource Usage Details**

| Sample                               | RAM\_ILM\_N                | ROM                       | RAM                         |
| ------------------------------------ | -------------------------- | ------------------------- | --------------------------- |
| **samples/basic/blinky**             | 7590 B (5.79% of 128 KB)   | 33374 B (3.18% of 1 MB)   | 19200 B (14.65% of 128 KB)  |
| **samples/basic/blinky\_pwm**        | 7590 B (5.79% of 128 KB)   | 42710 B (4.07% of 1 MB)   | 19272 B (14.70% of 128 KB)  |
| **samples/basic/button**             | 7590 B (5.79% of 128 KB)   | 34014 B (3.24% of 1 MB)   | 19216 B (14.66% of 128 KB)  |
| **samples/basic/fade\_led**          | 7590 B (5.79% of 128 KB)   | 47374 B (4.52% of 1 MB)   | 22680 B (17.30% of 128 KB)  |
| **samples/hello\_world**             | 7590 B (5.79% of 128 KB)   | 32898 B (3.14% of 1 MB)   | 19200 B (14.65% of 128 KB)  |
| **samples/bluetooth/peripheral\_ht** | 18544 B (14.15% of 128 KB) | 205960 B (19.64% of 1 MB) | 43784 B (33.40% of 128 KB)  |
| **samples/net/openthread/cli**       | 12078 B (9.21% of 128 KB)  | 396650 B (37.83% of 1 MB) | 91016 B (69.44% of 128 KB)  |
| **samples/net/openthread/coprocessor**| 12042 B (9.19% of 128 KB) | 137202 B (13.08% of 1 MB) | 56088 B (42.79% of 128 KB)  |
| **samples/crypto/mbedtls**           | 7590 B (5.79% of 128 KB)   | 136338 B (13.00% of 1 MB) | 27784 B (21.20% of 128 KB)  |
| **samples/drivers/adc/adc\_dt**      | 7590 B (5.79% of 128 KB)   | 43606 B (4.16% of 1 MB)   | 20084 B (15.32% of 128 KB)  |
| **samples/drivers/spi\_flash**       | 7834 B (5.98% of 128 KB)   | 41078 B (3.92% of 1 MB)   | 19360 B (14.77% of 128 KB)  |
| **samples/drivers/watchdog**         | 7590 B (5.79% of 128 KB)   | 41730 B (3.98% of 1 MB)   | 19272 B (14.70% of 128 KB)  |
| **samples/sensor/sht3xd**            | 7666 B (5.85% of 128 KB)   | 46978 B (4.48% of 1 MB)   | 19244 B (14.68% of 128 KB)  |
| **tests/drivers/flash/common**       | 7590 B (5.79% of 128 KB)   | 56994 B (5.44% of 1 MB)   | 25216 B (19.24% of 128 KB)  |
| **tests/drivers/hwinfo/api**         | 7866 B (6.00% of 128 KB)   | 52114 B (4.97% of 1 MB)   | 21568 B (16.46% of 128 KB)  |
| **bootloader/mcuboot/boot/zephyr**   | 23522 B (17.95% of 128 KB) | 83473 B (7.96% of 1 MB)   | 92212 B (70.35% of 128 KB)  |

> 📌 **Note:** `retention_echo_client` (PM mode) is excluded from this table as
> this release focuses on always-on (non-PM) device support. Memory usage data
> is extracted from the linker output (`Memory region` summary) of the build
> logs. `RAM_ILM_N` (instruction RAM) and `RAM` (data RAM) are 128 KB each;
> `ROM` is the application slot0 partition (1 MB). For a detailed RAM/ROM symbol
> breakdown, run `west build -t ram_report` / `west build -t rom_report` in the
> build directory.

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
- [Zephyr v4.1.0 Migration Guide](https://github.com/zephyrproject-rtos/zephyr/blob/main/doc/releases/migration-guide-4.1.rst)
