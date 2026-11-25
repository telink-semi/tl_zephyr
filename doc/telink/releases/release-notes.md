# Telink Zephyr SDK Release Note

[![Version](https://img.shields.io/badge/Version-tl_v1.5.1--alpha--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/tl_zephyr/releases/tag/tl_v1.5.1-alpha-v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](../../../LICENSE)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)

***

- **Release Type:** Alpha Release
- **Tag Version:** tl\_v1.5.1-alpha-v4.1.0

<!-- - **Branch:** release-v1.5-v4.1-branch -->
<!-- - **Target Commit:** de75d4c1ec8fcfbb968768d3d0a6a776e0f17fec -->

***

## 📖 Introduction

This patch release is the follow-up to tl\_v1.5.0 and backports the Telink
changes from `dev-tlk_v4.1` onto `release-v1.5-v4.1-branch`. It adds the TL323x
unified 160 KB RAM layout for concurrent BLE + Thread builds, TL523X BLE
support, BLE central role support, low-battery-detect demos, and a set of
GPIO/SWS/SoC fixes, together with the BLE SDK v4.0.4.8 and hal_telink updates.

### Based on Upstream Zephyr v4.1.0

This release is based on upstream Zephyr **v4.1.0** (tag [`v4.1.0`](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)). For the full list of upstream changes, see:

- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://docs.zephyrproject.org/latest/releases/migration-guide-4.1.html) — recommended reading when upgrading from a previous Zephyr version, as upstream v4.1.0 introduced several API and Kconfig changes (Bluetooth HCI driver API rewrite, pipe API rework, removed/deprecated options, etc.).

For environment setup, see the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md).

***

## 🆕 Changes in tl\_v1.5.1

This patch release backports the following changes on top of tl\_v1.5.0:

- **TL323x unified 160 KB RAM:** Select the merged 160 KB RAM mapping only for
  BLE + Thread coexistence builds (`CONFIG_IEEE802154_TLX_BLE_COEXIST`); MCUboot
  and non-coexistence builds keep the previous layout ([#900](https://github.com/telink-semi/tl_zephyr/pull/900)).
- **TL523X BLE support:** Add TL523X BLE functions and the dedicated TL523X
  flash driver ([#892](https://github.com/telink-semi/tl_zephyr/pull/892)).
- **BLE central role:** Add BLE central role support ([#893](https://github.com/telink-semi/tl_zephyr/pull/893)).
- **Low-battery demos:** Add the BLE peripheral low-battery-detect samples
  ([#895](https://github.com/telink-semi/tl_zephyr/pull/895)).
- **GPIO/SWS fixes:**
  - Fix GPIO interrupt enable placement in the Telink GPIO drivers
    ([#889](https://github.com/telink-semi/tl_zephyr/pull/889)).
  - Gate `gpio_shutdown(GPIO_ALL)` with `CONFIG_PM` for always-on products and
    document the SWS pull-up choice ([#896](https://github.com/telink-semi/tl_zephyr/pull/896)).
  - Drop the redundant SWS pull-up from the restore hooks
    ([#897](https://github.com/telink-semi/tl_zephyr/pull/897)).
- **Peak BTR:** Update the TL521X/TL323X peak BTR configuration
  ([#891](https://github.com/telink-semi/tl_zephyr/pull/891)).
- **Dependencies:** Update `telink_ble_sdk` to `a508cc0`
  (`release/V4.0.4.8`) and `hal_telink` to `3cc9054`.

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

| Property         | Value                            |
| ---------------- | -------------------------------- |
| **Tag Name**     | tl\_v1.5.1-alpha-v4.1.0          |
| **Release Type** | Alpha Release                    |

### Chip & Hardware Versions

📦 **Chip Versions**

| Chip Family | Versions |
| ----------- | -------- |
| TL323X      | A1       |

🔧 **Hardware EVK Versions**

| Chip    | EVK Version      |
| ------- | ---------------- |
| TL323X  | C1T388A20\_V1\_2   |

> **Note:** TL323X is validated in this patch release.

***

## ✨ Highlights

| Category             | Details                                                                 |
| -------------------- | ----------------------------------------------------------------------- |
| **Power Management** | BLE + Thread concurrent power management with a new `ot_ble_pm` sample  |
| **Memory Layout**    | TL323x unified 160 KB RAM layout for concurrent BLE + Thread builds     |
| **New Drivers**      | Reworked NFC ST25DV tag driver and new ST25DV tag sample                |
| **New SoC Features** | TL523X BLE support and BLE central role support                         |
| **Demos**            | BLE peripheral low-battery-detect samples                               |
| **Networking**       | IEEE 802.15.4 ED scan, TL521X feature sync, RX parsing fix              |
| **Flashing**         | Auto-detect SBOOT firmware for the MCUboot start offset                 |
| **GPIO/SWS**         | GPIO interrupt and shutdown gating fixes                                |

This patch release extends the tl\_v1.5.0 line with the TL323x unified RAM
layout, TL523X/central BLE support, low-battery demo samples, and GPIO/SWS/SoC
fixes backported from `dev-tlk_v4.1`.

***

## 🆕 New Features

- ✅ TL323x unified 160 KB RAM layout for BLE + Thread coexistence builds ([#900](https://github.com/telink-semi/tl_zephyr/pull/900));
- ✅ TL523X BLE functions support ([#892](https://github.com/telink-semi/tl_zephyr/pull/892));
- ✅ BLE central role support ([#893](https://github.com/telink-semi/tl_zephyr/pull/893));
- ✅ BLE peripheral low-battery-detect demos ([#895](https://github.com/telink-semi/tl_zephyr/pull/895));
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
| [#889](https://github.com/telink-semi/tl_zephyr/pull/889) | GPIO | Fix GPIO interrupt enable placement in the Telink GPIO drivers |
| [#896](https://github.com/telink-semi/tl_zephyr/pull/896) | SoC/GPIO | Gate `gpio_shutdown(GPIO_ALL)` with `CONFIG_PM`; note the SWS pull-up choice |
| [#897](https://github.com/telink-semi/tl_zephyr/pull/897) | SoC/GPIO | Drop the redundant SWS pull-up from the restore hooks |
| [#891](https://github.com/telink-semi/tl_zephyr/pull/891) | RF | Update the TL521X/TL323X peak BTR configuration |
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
| `gpio_shutdown(GPIO_ALL)` in the SoC init/restore paths is now gated by `CONFIG_PM` ([#896](https://github.com/telink-semi/tl_zephyr/pull/896)) | Always-on (`CONFIG_PM=n`) builds keep their pin mux and PWM outputs after a retention reset; only power-managed builds reset all pins | No action required; always-on products no longer need to re-apply pin mux after a retention reset |
| The TL323x unified 160 KB RAM mapping is selected only when `CONFIG_IEEE802154_TLX_BLE_COEXIST` is set ([#900](https://github.com/telink-semi/tl_zephyr/pull/900)) | MCUboot and non-coexistence builds keep the previous 96 KB / 64 KB split | No action required |
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
| **Telink BLE SDK**        | [telink-semi/tl\_ble\_sdk\_zephyr](https://github.com/telink-semi/tl_ble_sdk_zephyr) | [`a508cc0`](https://github.com/telink-semi/tl_ble_sdk_zephyr/commit/a508cc070bee70d5596624377e0b078b29c29fc2) | Updated from `eb0d120` (`release/V4.0.4.8`); ships the tl323x multirole library with the PM symbols required by concurrent mode |
| **Telink HAL Zephyr**     | [telink-semi/hal\_telink](https://github.com/telink-semi/hal_telink) | [`3cc9054`](https://github.com/telink-semi/hal_telink/commit/3cc9054edd5edcaa1a8b5870099af81433afaf58) | Updated from `81ded24`; adds peripheral\_ht and central support |
| **MCUBoot**               | [telink-semi/tl\_mcuboot](https://github.com/telink-semi/tl_mcuboot) | [`ce0da85`](https://github.com/telink-semi/tl_mcuboot/commit/ce0da85c39c749df49b0ec62b33d2ecdea24c927) | Unchanged since tl\_v1.4.1; bootloader required for OTA/DFU    |
| **OpenThread Telink**     | [telink-semi/tl\_openthread](https://github.com/telink-semi/tl_openthread) | [`542aaab`](https://github.com/telink-semi/tl_openthread/commit/542aaab44e1308e1a8a24573dfbd413fade342ee) | Unchanged since tl\_v1.4.1; OpenThread source adapted for Telink |
| **OpenThread Telink Lib** | [telink-semi/tl\_openthread\_libs](https://github.com/telink-semi/tl_openthread_libs) | [`bb57bac`](https://github.com/telink-semi/tl_openthread_libs/commit/bb57bac0857cdf4f408894ffdd9c8469d25c66cb) | Updated from `f69c186`; pre-built OpenThread library for Telink |
| **Telink XZ (LZMA)**      | [telink-semi/tl\_xz](https://github.com/telink-semi/tl_xz) | [`831f338`](https://github.com/telink-semi/tl_xz/commit/831f338fd6784661d3bec62fd01060ee4d7d373d) | Unchanged since tl\_v1.4.1; LZMA compression library module (`modules/lib/lzma`) |
| **NFC ST25DV**            | [telink-semi/nfc\_st25dv](https://github.com/telink-semi/nfc_st25dv) | [`f7e03b0`](https://github.com/telink-semi/nfc_st25dv/commit/f7e03b0616e454fed9a02c7c612a17e20b8af616) | New in tl\_v1.5.0; NFC subsystem module |
| **Telink ST25DV**         | [telink-semi/tl\_st25dv](https://github.com/telink-semi/tl_st25dv) | [`1b6e0c4`](https://github.com/telink-semi/tl_st25dv/commit/1b6e0c496b7a23392dcc48d8bce2d8955426dde5) | New in tl\_v1.5.0; Telink ST25DV tag module |

### Dependency Changes Since tl\_v1.5.0

- `telink_ble_sdk`: `eb0d120` → `a508cc0`
- `hal_telink`: `81ded24` → `3cc9054`
- `tl_mcuboot`, `tl_openthread`, `tl_openthread_libs`, `tl_xz`, `nfc_st25dv`, and `telink_st25dv` remain unchanged from tl\_v1.5.0.

***

## ⚠️ Known Issues and Limitations

- The TL323x unified 160 KB RAM layout is only selected for BLE + Thread
  coexistence builds; MCUboot and non-coexistence builds keep the 96 KB / 64 KB
  split.
- BLE post-commissioning teardown/resume is a Matter-side feature; see the
  Telink Matter SDK release notes.

***

## 📌 Important Notes

- Use `west update` to automatically pull `telink_ble_sdk` along with other modules.
- Alternatively, manually fetch `telink_ble_sdk` by running `./hal_v2/fetch_sdk.sh {REPO_URL} {COMMIT_HASH}` inside `modules/hal/telink/` to pull or update the BLE stack to the version pinned by this release, where `{COMMIT_HASH}` is the `telink_ble_sdk` revision in `west.yml`. For this release, that is: `./hal_v2/fetch_sdk.sh https://github.com/telink-semi/tl_ble_sdk_zephyr.git a508cc070bee70d5596624377e0b078b29c29fc2`
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
| tl3238x  | TL323X      |

> **Note:** tl3238x is validated in this release.

***

### Zephyr Samples Support Matrix

The resource usage tables were generated from local builds. All listed samples
compile successfully, but they were not functionally validated in this release
(functional testing was covered in a previous release), so the per-sample
support matrix is omitted here.

***

### TL323X (tl3238x)

📈 **Resource Usage Details**

| Sample                                | RAM\_ILM\_N                 | ROM                       | RAM                        |
| ------------------------------------- | --------------------------- | ------------------------- | -------------------------- |
| **bootloader/mcuboot/boot/zephyr**    | 23690 B (36.15% of 64 KB)   | 73997 B (7.06% of 1 MB)   | 92292 B (93.88% of 96 KB)  |
| **samples/basic/blinky**              | 9668 B (14.75% of 64 KB)    | 45564 B (4.35% of 1 MB)   | 19520 B (19.86% of 96 KB)  |
| **samples/basic/blinky\_pwm**         | 9668 B (14.75% of 64 KB)    | 54892 B (5.23% of 1 MB)   | 19600 B (19.94% of 96 KB)  |
| **samples/basic/button**              | 9668 B (14.75% of 64 KB)    | 46204 B (4.41% of 1 MB)   | 19536 B (19.87% of 96 KB)  |
| **samples/basic/fade\_led**           | 9668 B (14.75% of 64 KB)    | 59552 B (5.68% of 1 MB)   | 23000 B (23.40% of 96 KB)  |
| **samples/hello\_world**              | 9668 B (14.75% of 64 KB)    | 45088 B (4.30% of 1 MB)   | 19520 B (19.86% of 96 KB)  |
| **samples/bluetooth/peripheral\_ht**  | 20360 B (31.07% of 64 KB)   | 214368 B (20.44% of 1 MB) | 43280 B (44.03% of 96 KB)  |
| **samples/net/openthread/cli**        | 14336 B (21.88% of 64 KB)   | 407988 B (38.91% of 1 MB) | 90528 B (92.09% of 96 KB)  |
| **samples/net/openthread/coprocessor**| 14312 B (21.84% of 64 KB)   | 148620 B (14.17% of 1 MB) | 56432 B (57.41% of 96 KB)  |
| **samples/net/sockets/echo\_client**  | 22010 B (33.58% of 64 KB)   | 277200 B (26.44% of 1 MB) | 60732 B (61.78% of 96 KB)  |
| **samples/crypto/mbedtls**            | 9668 B (14.75% of 64 KB)    | 148408 B (14.15% of 1 MB) | 27272 B (27.74% of 96 KB)  |
| **samples/drivers/adc/adc\_dt**       | 9668 B (14.75% of 64 KB)    | 52020 B (4.96% of 1 MB)   | 20404 B (20.76% of 96 KB)  |
| **samples/drivers/spi\_flash**        | 9912 B (15.12% of 64 KB)    | 53748 B (5.13% of 1 MB)   | 19668 B (20.01% of 96 KB)  |
| **samples/drivers/watchdog**          | 9668 B (14.75% of 64 KB)    | 53912 B (5.14% of 1 MB)   | 19600 B (19.94% of 96 KB)  |
| **samples/sensor/sht3xd**             | 9740 B (14.86% of 64 KB)    | 56048 B (5.35% of 1 MB)   | 19576 B (19.91% of 96 KB)  |
| **tests/drivers/flash/common**        | 9668 B (14.75% of 64 KB)    | 69368 B (6.62% of 1 MB)   | 25536 B (25.98% of 96 KB)  |
| **tests/drivers/hwinfo/api**          | 9920 B (15.14% of 64 KB)    | 64456 B (6.15% of 1 MB)   | 21872 B (22.25% of 96 KB)  |

> 📌 **Note:** `RAM_ILM_N` (instruction RAM) is 64 KB; `RAM` (data RAM) is
> 96 KB; `ROM` is the application slot0 partition (1 MB). These numbers use the
> default (non-coexistence) layout; the unified 160 KB layout only applies to
> concurrent BLE + Thread builds.

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
