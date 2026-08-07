# Telink Zephyr SDK Release Note

[![Version](https://img.shields.io/badge/Version-tl_v1.2.0--alpha--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/zephyr/releases/tag/tl_v1.2.0-alpha-v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](LICENSE)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)

---

- **Release Type:** Pre-Release (alpha)
- **Branch:** pre_release-v1.2-v4.1-branch
- **Tag Version:** tl_v1.2.0-alpha-v4.1.0
<!-- - **Target Commit:** 8e3ccc07900692fe5a9990cd517203de61b2eefc -->

---

## 📖 Introduction

This release is based on the latest commit of `pre_release-v1.2-v4.1-branch` branch, adding support for the new TL521X chip series, running BLE concurrently with an active Thread network on TL7218X and TL3238X, and integrating a Channel Sounding (CS) RAS reflector on TL7218X.

### Based on Upstream Zephyr v4.1.0

This release is based on upstream Zephyr **v4.1.0** (tag [`v4.1.0`](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)). For the full list of upstream changes, see:

- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html) — recommended reading when upgrading from a previous Zephyr version, as upstream v4.1.0 introduced several API and Kconfig changes (Bluetooth HCI driver API rewrite, pipe API rework, removed/deprecated options, etc.).

For environment setup, see the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md).

---

## 📋 Version Information

### Zephyr SDK & Toolchain

| Component | Version |
|-----------|---------|
| **Host OS (recommended)** | Ubuntu 24.04 LTS |
| **Zephyr SDK Version** | Telink Zephyr v4.1.0 |
| **Zephyr SDK** | 0.17.0 |
| **Toolchain** | riscv64-zephyr-elf |

> **Note:** Only the `riscv64-zephyr-elf` toolchain (from Zephyr SDK 0.17.0) is validated for Telink SoCs. The experimental IAR compiler support introduced in upstream Zephyr v4.1.0 is **not** tested with this release. GCC-based `riscv64-zephyr-elf` is the recommended and only supported toolchain. Ubuntu 24.04 LTS is the recommended host OS.

### Telink SDK

<!-- | Property | Value |
|----------|-------|
| **Branch** | release-v1.0-v4.1-branch |
| **Target Commit** | e08fc42546e58d808bfd39f35c8df296f5617a44 |
| **Tag Name** | tl_v1.0.1-rc1-v4.1.0 |
| **Release Type** | Pre-Release (rc1) | -->

| Property | Value |
|----------|-------|
| **Branch** | pre_release-v1.2-v4.1-branch |
| **Tag Name** | tl_v1.2.0-alpha-v4.1.0 |
| **Release Type** | Pre-Release (alpha) |

### Chip & Hardware Versions

📦 **Chip Versions**

<!-- | Chip Family | Versions |
|-------------|----------|
| TLSR921X/TLSR951X(B91) | A2 |
| TLSR922X/TLSR952X(B92) | A3/A4 |
| TL721X | A2/A3 |
| TL321X | A1/A2/A3 |
| TL322X | A1 |
| TL323X | A0 | -->

| Chip Family | Versions |
|-------------|----------|
| TL521X | A0 |
| TL721X | A2/A3 |
| TL323X | A0 |

🔧 **Hardware EVK Versions**

<!-- | Chip | EVK Version |
|------|-------------|
| TLSR921X | C1T213A20_V1.3 |
| TLSR952X | C1T266A20_V1.3 |
| TL721X | C1T315A20_V1.2 |
| TL321X | C1T331A20_V1.0/C1T335A20_V1.3 |
| TL322X | C1T382A20_V1.2 |
| TL323X | C1T388A20_V1.1 | -->

| Chip | EVK Version |
|------|-------------|
| TL521X | C1T416A20_V1.0 |
| TL721X | C1T315A20_V1.2 |
| TL323X | C1T388A20_V1.1 |

---

## ✨ Highlights

| Category | Details |
|----------|---------|
| **New Chips** | Support for TL521X series |
| **New Features** | BLE + Thread concurrent mode on TL7218X and TL3238X |
| **New Features** | Channel Sounding (CS) RAS reflector on TL7218X |

This release adds support for the new TL521X chip series, and enables **BLE concurrent with an active Thread network** on TL7218X and TL3238X, plus a **Channel Sounding (CS) RAS reflector** on TL7218X.

**Supported configurations:**

| SoC | Configuration |
|-----|---------------|
| TL521X | BLE-only or Thread-only |
| TL323X | BLE + Thread concurrent |
| TL721X | BLE + Thread concurrent, with optional Channel Sounding (CS) |
| Other TLX SoCs | BLE-only or Thread-only (same as previous releases) |

---

## 🔒 Security

This release includes all security fixes inherited from upstream Zephyr v4.1.0, addressing the following CVEs:

- [CVE-2025-1673](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jjhx-rrh4-j8mx)
- [CVE-2025-1674](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x975-8pgf-qh66)
- [CVE-2025-1675](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2m84-5hfw-m8v4)

More detailed information can be found in the [Zephyr Security Vulnerabilities](https://docs.zephyrproject.org/latest/security/vulnerabilities.html) page.

---

## 🆕 New Features

- ✅ Added support for TL521X series chips
- ✅ Added BLE + Thread concurrent mode support on TL7218X and TL3238X
- ✅ Added Channel Sounding (CS) RAS reflector on TL7218X

---

## 🐛 Bug Fixes

| Issue | Component | Description |
|-------|-----------|-------------|
| **CS Amazon commissioning** | RF/SOC | Fixed Amazon commissioning failure when Channel Sounding is enabled |

---

## 🔧 API & Kconfig Changes

### Telink-Specific Changes

The following Telink-specific Kconfig changes may affect existing applications when upgrading to this release:

| Change | Impact | Migration |
|--------|--------|-----------|
| **`CONFIG_IEEE802154_TLX_BLE_COEXIST` concurrent** | Thread (IEEE 802.15.4) and BLE can now coexist on TL721X and TL323X | No migration needed for BLE-only or Thread-only uses; enable both stacks for concurrent mode |
| **`CONFIG_BT_CHANNEL_SOUNDING` added** | New Kconfig for Bluetooth 6.0 Channel Sounding (CS) feature on TL721X | Enable `CONFIG_BT_CHANNEL_SOUNDING=y` to use CS; TL721X only |


### Upstream Inherited Changes

This release inherits all upstream Zephyr v4.1.0 API and Kconfig changes.

For the complete list, see the upstream [Zephyr v4.1.0 Release Notes — API Changes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html#api-changes) and the [Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html).

---

## 📦 Updates / Dependencies

| Component | Repository | Commit | Notes |
|-----------|------------|--------|-------|
| **Telink BLE SDK** | [telink-semi/tl_ble_sdk_zephyr](https://github.com/telink-semi/tl_ble_sdk_zephyr) | [`53eb98b`](https://github.com/telink-semi/tl_ble_sdk_zephyr/commit/53eb98b32ea79ed7ab38f5daabde0a78a7880cd9) | Required by all Telink SoCs; fetch via `./hal_v2/fetch_sdk.sh` (hal_v2) or `west blobs fetch hal_telink` (hal_v1) |
| **Telink HAL Zephyr** | [telink-semi/hal_telink](https://github.com/telink-semi/hal_telink) | [`bdf0d79`](https://github.com/telink-semi/hal_telink/commit/bdf0d7927d31809340610b5e1575667e2d862110) | Contains both hal_v1 and hal_v2 sources |
| **MCUBoot** | [telink-semi/mcuboot](https://github.com/telink-semi/mcuboot) | [`ce0da85`](https://github.com/telink-semi/mcuboot/commit/ce0da85c39c749df49b0ec62b33d2ecdea24c927) | Bootloader; required for OTA/DFU |
| **OpenThread Telink** | [telink-semi/openthread](https://github.com/telink-semi/openthread) | [`542aaab`](https://github.com/telink-semi/openthread/commit/542aaab44e1308e1a8a24573dfbd413fade342ee) | OpenThread source adapted for Telink |
| **OpenThread Telink Lib** | [telink-semi/openthread_telink_lib](https://github.com/telink-semi/openthread_telink_lib) | [`308dae2`](https://github.com/telink-semi/openthread_telink_lib/commit/308dae2f80084f87073cfd4fbd30f1be0799be7b) | Pre-built OpenThread library for Telink |

---

## ⚠️ Known Issues and Limitations

- **Pre-release:** This is a pre-release version for demonstration, development and testing purposes. Not recommended for production use.
- **Concurrent BLE + Thread:** Concurrent mode is supported on TL721X and TL323X only. Other TLX SoCs remain BLE-only or Thread-only.
- **Channel Sounding:** CS RAS reflector is supported on TL721X only. BLE OTA is not currently supported when Channel Sounding is enabled.

---

## 📌 Important Notes

- Use `west update` to automatically pull `tl_ble_sdk` along with other modules.
- Alternatively, manually fetch `tl_ble_sdk` by running `./hal_v2/fetch_sdk.sh` inside `modules/hal/telink/` to pull or update the BLE stack to the version pinned by this release.
- For full environment setup instructions, refer to the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md) or the [Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/).

---

## 📊 Resource Usage (Code Size)

This section shows the RAM and ROM usage for various Zephyr samples on Telink platforms.

**Build environment:**

| Property | Value |
|----------|-------|
| Zephyr SDK | 0.17.0 |
| Toolchain | `riscv64-zephyr-elf` |
| Extra CMake flag | `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` |
| Debug logging | Enabled (per sample default) |
| Reproduce | `west build -p auto -b <board> <sample> -- -DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` |

> **Note:** The numbers below are from CI builds with the configuration above.

### Supported Boards

<!-- | Board | Chip Family |
|-------|-------------|
| tlsr9518adk80d | TLSR951X/B91 |
| tlsr9528a | TLSR952X/B92 |
| tl3218x | TL321X |
| tl3228x | TL322X |
| tl3238x | TL323X |
| tl7218x | TL721X |
| tlsr9118bdk40d | TLSR9118BDK40D | -->

| Board | Chip Family |
|-------|-------------|
| tl3238x | TL323X |
| tl7218x | TL721X |
| tl5218x | TL521X |

---

### Zephyr Samples Support Matrix

The table below summarizes build and test status for Zephyr samples across Telink
chip families in this release. Samples are located under `samples/` in the Zephyr
tree (tests are under `tests/`).

> ✅ = Supported and Tested &nbsp;&nbsp;
> 🟡 = Supported but Untested
> &nbsp;&nbsp; (builds successfully, not functionally validated)
> &nbsp;&nbsp; · = Untested (not built or not applicable)

<!-- | Sample | B91 (TLSR951X) | B92 (TLSR952X) | TL321X | TL322X | TL323X | TL721X | W91 (TLSR911X) |
|--------|:------------:|:------------:|:------------:|:------------:|:------------:|:------------:|:------------:|
| **basic/blinky** | 🟡 | 🟡 | 🟡 | 🟡 | ✅ | ✅ | 🟡 |
| **basic/button** | · | · | 🟡 | 🟡 | ✅ | ✅ | 🟡 |
| **basic/fade_led** | · | · | 🟡 | · | ✅ | ✅ | · |
| **hello_world** | · | · | 🟡 | 🟡 | ✅ | ✅ | · |
| **bluetooth/peripheral_ht** | 🟡 | 🟡 | 🟡 | · | ✅ | ✅ | 🟡 |
| **net/openthread/cli** | 🟡 | 🟡 | · | · | ✅ | ✅ | 🟡 |
| **net/openthread/coprocessor** | · | · | · | · | · | · | · |
| **net/sockets/echo_client** | · | · | · | · | ✅ | ✅ | · |
| **net/sockets/echo_server** | · | · | · | · | · | 🟡 | · |
| **crypto/mbedtls** | · | · | · | · | ✅ | ✅ | 🟡 |
| **drivers/adc/adc_dt** | · | · | 🟡 | 🟡 | ✅ | 🟡 | · |
| **drivers/spi_flash** | · | · | 🟡 | · | ✅ | 🟡 | · |
| **drivers/spi_flash_at45** | · | · | · | · | · | · | 🟡 |
| **drivers/uart/echo_bot** | · | · | · | · | · | · | 🟡 |
| **drivers/watchdog** | · | · | 🟡 | 🟡 | ✅ | ✅ | · |
| **sensor/mpu6050** | · | · | · | · | · | · | 🟡 |
| **sensor/sht3xd** | · | · | 🟡 | 🟡 | ✅ | 🟡 | · |
| **subsys/nvs** | · | · | · | · | · | · | 🟡 |
| **subsys/shell/devmem_load** | · | · | · | · | · | · | 🟡 |
| **usb/cdc_acm** | · | · | · | · | · | · | · |
| **usb/console** | · | · | · | · | · | · | · |
| **retention/basic** | · | · | · | · | · | · | · |
| **common** | · | · | · | · | · | · | · |
| **factorydata** | · | · | · | · | · | · | · |
| **smp_svr** | · | · | · | · | · | · | · |
| **ml3m_button** | · | · | · | · | · | · | · |
| **boards/tlsr9x/gpio-kbd-matrix** | 🟡 | 🟡 | 🟡 | · | · | · | 🟡 |
| **boards/tlsr9x/key_matrix** | · | · | · | · | · | · | 🟡 |
| **boards/tlsr9x/key_pool** | · | · | · | · | · | · | 🟡 |
| **boards/tlsr9x/led_pool** | · | · | · | · | · | · | 🟡 |
| **boards/tlsr9x/pwm_pool** | · | · | · | · | · | · | 🟡 |
| **boards/tlsr9x/sock_simple** | · | · | · | · | · | · | 🟡 |
| **tests/drivers/adc/adc_api** | · | · | · | · | · | · | 🟡 | -->

| Sample | TL521X | TL323X | TL721X |
|--------|:------------:|:------------:|:------------:|
| **basic/blinky** | ✅ | ✅ | ✅ |
| **basic/button** | ✅ | ✅ | ✅ |
| **basic/fade_led** | ✅ | ✅ | ✅ |
| **hello_world** | ✅ | ✅ | ✅ |
| **bluetooth/peripheral_ht** | ✅ | ✅ | ✅ |
| **net/openthread/cli** | ✅ | ✅ | ✅ |
| **net/sockets/echo_client** | ✅ | ✅ | ✅ |
| **net/sockets/echo_server** | · | · | 🟡 |
| **crypto/mbedtls** | ✅ | ✅ | ✅ |
| **drivers/adc/adc_dt** | ✅ | ✅ | 🟡 |
| **drivers/spi_flash** | · | ✅ | 🟡 |
| **drivers/watchdog** | ✅ | ✅ | ✅ |
| **sensor/sht3xd** | ✅ | ✅ | 🟡 |

#### Notes on Sample Support

<!-- - **Tested combinations (✅):** All TL521X and TL323X samples that build successfully have been -->
  <!-- functionally validated. On TL721X, the core bring-up samples (`blinky`, `button`,
  `fade_led`, `hello_world`) plus BLE (`peripheral_ht`), Thread (`openthread/cli`),
  crypto (`mbedtls`), networking (`echo_client`), and driver (`watchdog`) samples
  have been functionally validated in this release.
- **Supported but untested (🟡):** All other build targets listed in the table
  compile successfully but have not been functionally validated. Use with caution.
- **Untested (·):** Combinations marked · are not built in this release, either
  because the sample is not applicable to that chip family or because the
  build failed. -->


---
<!--
### TLSR951X (tlsr9518adk80d)

📈 **Resource Usage Details**

| Sample | RAMILM | ROM | RAM |
|--------|--------|-----|-----|
| **samples/basic/blinky** | 21248 B (16.21% of 128 KB) | 23848 B (2.27% of 1 MB) | 868 B (0.66% of 128 KB) |
| **samples/bluetooth/peripheral_ht** | 65856 B (50.24% of 128 KB) | 210412 B (20.07% of 1 MB) | 12556 B (9.58% of 128 KB) |
| **samples/boards/tlsr9x/gpio-kbd-matrix** | 24512 B (18.70% of 128 KB) | 32388 B (3.09% of 1 MB) | 2736 B (2.09% of 128 KB) |
| **samples/net/openthread/cli** | 53472 B (40.80% of 128 KB) | 552436 B (52.68% of 1 MB) | 88912 B (67.83% of 128 KB) |

---

### TLSR952X (tlsr9528a)

📈 **Resource Usage Details**

| Sample | RAMILM | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 24016 B (9.16% of 256 KB) | 29630 B (2.83% of 1 MB) | 972 B (0.37% of 256 KB) |
| **samples/bluetooth/peripheral_ht** | 76720 B (29.27% of 256 KB) | 231492 B (22.08% of 1 MB) | 25592 B (9.76% of 256 KB) |
| **samples/boards/tlsr9x/gpio-kbd-matrix** | 27280 B (10.41% of 256 KB) | 38170 B (3.64% of 1 MB) | 2840 B (1.08% of 256 KB) |
| **samples/net/openthread/cli** | 57712 B (22.02% of 256 KB) | 561774 B (53.57% of 1 MB) | 89048 B (33.97% of 256 KB) |

---

### TL321X (tl3218x)

📈 **Resource Usage Details**

| Sample | RAM_ILM_N | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 5144 B (15.70% of 32 KB) | 27820 B (2.65% of 1 MB) | 18960 B (19.29% of 96 KB) |
| **samples/basic/button** | 5144 B (15.70% of 32 KB) | 28460 B (2.71% of 1 MB) | 18984 B (19.31% of 96 KB) |
| **samples/basic/fade_led** | 5144 B (15.70% of 32 KB) | 41680 B (3.97% of 1 MB) | 22432 B (22.82% of 96 KB) |
| **samples/bluetooth/peripheral_ht** | 14676 B (44.79% of 32 KB) | 191866 B (18.30% of 1 MB) | 42024 B (42.75% of 96 KB) |
| **samples/boards/tlsr9x/gpio-kbd-matrix** | 5144 B (15.70% of 32 KB) | 36356 B (3.47% of 1 MB) | 24084 B (24.50% of 96 KB) |
| **samples/drivers/adc/adc_dt** | 5592 B (17.07% of 32 KB) | 36760 B (3.51% of 1 MB) | 20168 B (20.52% of 96 KB) |
| **samples/drivers/spi_flash** | 5442 B (16.61% of 32 KB) | 35054 B (3.34% of 1 MB) | 19116 B (19.45% of 96 KB) |
| **samples/drivers/watchdog** | 5144 B (15.70% of 32 KB) | 35700 B (3.40% of 1 MB) | 19040 B (19.37% of 96 KB) |
| **samples/hello_world** | 5144 B (15.70% of 32 KB) | 27292 B (2.60% of 1 MB) | 18960 B (19.29% of 96 KB) |
| **samples/net/openthread/cli** | 9812 B (29.94% of 32 KB) | 439026 B (41.87% of 1 MB) | 85108 B (86.58% of 96 KB) |
| **samples/sensor/sht3xd** | 5144 B (15.70% of 32 KB) | 41000 B (3.91% of 1 MB) | 18996 B (19.32% of 96 KB) |

---

### TL322X (tl3228x)

📈 **Resource Usage Details**

| Sample | RAMILM | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 26512 B (5.06% of 512 KB) | 34782 B (3.32% of 1 MB) | 1160 B (0.89% of 128 KB) |
| **samples/basic/button** | 26512 B (5.06% of 512 KB) | 35410 B (3.38% of 1 MB) | 1176 B (0.90% of 128 KB) |
| **samples/drivers/adc/adc_dt** | 26528 B (5.06% of 512 KB) | 40842 B (3.89% of 1 MB) | 2044 B (1.56% of 128 KB) |
| **samples/drivers/watchdog** | 26512 B (5.06% of 512 KB) | 42654 B (4.07% of 1 MB) | 1236 B (0.94% of 128 KB) |
| **samples/hello_world** | 26512 B (5.06% of 512 KB) | 34250 B (3.27% of 1 MB) | 1160 B (0.89% of 128 KB) |
| **samples/sensor/sht3xd** | 26512 B (5.06% of 512 KB) | 48326 B (4.61% of 1 MB) | 1224 B (0.93% of 128 KB) |

--- -->

### TL521X (tl5218x)

📈 **Resource Usage Details**

| Sample | RAM_ILM_N | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 5010 B (3.82% of 128 KB) | 30998 B (2.96% of 1 MB) | 19184 B (14.64% of 128 KB) |
| **samples/basic/button** | 5010 B (3.82% of 128 KB) | 31638 B (3.02% of 1 MB) | 19200 B (14.65% of 128 KB) |
| **samples/basic/fade_led** | 5010 B (3.82% of 128 KB) | 44994 B (4.29% of 1 MB) | 22664 B (17.29% of 128 KB) |
| **samples/bluetooth/peripheral_ht** | 18302 B (13.96% of 128 KB) | 203626 B (19.42% of 1 MB) | 42068 B (32.10% of 128 KB) |
| **samples/crypto/mbedtls** | 5010 B (3.82% of 128 KB) | 131334 B (12.52% of 1 MB) | 25892 B (19.75% of 128 KB) |
| **samples/drivers/adc/adc_dt** | 5014 B (3.83% of 128 KB) | 41294 B (3.94% of 1 MB) | 20084 B (15.32% of 128 KB) |
| **samples/drivers/watchdog** | 5010 B (3.82% of 128 KB) | 39350 B (3.75% of 1 MB) | 19256 B (14.69% of 128 KB) |
| **samples/hello_world** | 5010 B (3.82% of 128 KB) | 30522 B (2.91% of 1 MB) | 19184 B (14.64% of 128 KB) |
| **samples/net/openthread/cli** | 11882 B (9.07% of 128 KB) | 571552 B (54.51% of 1 MB) | 104912 B (80.04% of 128 KB) |
| **samples/net/sockets/echo_client** | 19146 B (14.61% of 128 KB) | 266164 B (25.38% of 1 MB) | 59032 B (45.04% of 128 KB) |
| **samples/sensor/sht3xd** | 5086 B (3.88% of 128 KB) | 44610 B (4.25% of 1 MB) | 19228 B (14.67% of 128 KB) |

---

### TL323X (tl3238x)

📈 **Resource Usage Details**

| Sample | RAM_ILM_N | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 9804 B (14.96% of 64 KB) | 45916 B (4.38% of 1 MB) | 19600 B (19.94% of 96 KB) |
| **samples/basic/button** | 9804 B (14.96% of 64 KB) | 46556 B (4.44% of 1 MB) | 19616 B (19.95% of 96 KB) |
| **samples/basic/fade_led** | 9804 B (14.96% of 64 KB) | 59912 B (5.71% of 1 MB) | 23080 B (23.48% of 96 KB) |
| **samples/bluetooth/peripheral_ht** | 20386 B (31.11% of 64 KB) | 212506 B (20.27% of 1 MB) | 42460 B (43.19% of 96 KB) |
| **samples/crypto/mbedtls** | 9804 B (14.96% of 64 KB) | 146452 B (13.97% of 1 MB) | 26308 B (26.76% of 96 KB) |
| **samples/drivers/adc/adc_dt** | 9804 B (14.96% of 64 KB) | 52380 B (5.00% of 1 MB) | 20484 B (20.84% of 96 KB) |
| **samples/drivers/spi_flash** | 10048 B (15.33% of 64 KB) | 53060 B (5.06% of 1 MB) | 19748 B (20.09% of 96 KB) |
| **samples/drivers/watchdog** | 9804 B (14.96% of 64 KB) | 53 KB (5.18% of 1 MB) | 19680 B (20.02% of 96 KB) |
| **samples/hello_world** | 9804 B (14.96% of 64 KB) | 45440 B (4.33% of 1 MB) | 19600 B (19.94% of 96 KB) |
| **samples/net/openthread/cli** | 14470 B (22.08% of 64 KB) | 449858 B (42.90% of 1 MB) | 80816 B (82.21% of 96 KB) |
| **samples/net/sockets/echo_client** | 22122 B (33.76% of 64 KB) | 274264 B (26.16% of 1 MB) | 59416 B (60.44% of 96 KB) |
| **samples/sensor/sht3xd** | 9876 B (15.07% of 64 KB) | 56400 B (5.38% of 1 MB) | 19656 B (20.00% of 96 KB) |

---

### TL721X (tl7218x)

📈 **Resource Usage Details**

<!-- | Sample | RAMILM | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 26608 B (10.15% of 256 KB) | 33630 B (3.21% of 1 MB) | 1012 B (0.39% of 256 KB) |
| **samples/basic/button** | 26608 B (10.15% of 256 KB) | 34258 B (3.27% of 1 MB) | 1028 B (0.39% of 256 KB) |
| **samples/basic/fade_led** | 28656 B (10.93% of 256 KB) | 47558 B (4.54% of 1 MB) | 2440 B (0.93% of 256 KB) |
| **samples/bluetooth/peripheral_ht** | 57232 B (21.83% of 256 KB) | 211328 B (20.15% of 1 MB) | 8548 B (3.26% of 256 KB) |
| **samples/crypto/mbedtls** | 31728 B (12.10% of 256 KB) | 133074 B (12.69% of 1 MB) | 2608 B (0.99% of 256 KB) |
| **samples/drivers/adc/adc_dt** | 28384 B (10.83% of 256 KB) | 43932 B (4.19% of 1 MB) | 1648 B (0.63% of 256 KB) |
| **samples/drivers/spi_flash** | 26784 B (10.22% of 256 KB) | 41758 B (3.98% of 1 MB) | 1284 B (0.49% of 256 KB) |
| **samples/drivers/watchdog** | 26608 B (10.15% of 256 KB) | 41502 B (3.96% of 1 MB) | 1088 B (0.42% of 256 KB) |
| **samples/hello_world** | 26608 B (10.15% of 256 KB) | 33098 B (3.16% of 1 MB) | 1012 B (0.39% of 256 KB) |
| **samples/net/openthread/cli** | 60768 B (23.18% of 256 KB) | 570272 B (54.39% of 1 MB) | 89184 B (34.02% of 256 KB) |
| **samples/net/sockets/echo_client** | N/A | 268848 B (25.64% of 1 MB) | 59200 B (45.17% of 128 KB) |
| **samples/net/sockets/echo_server** | 77564 B (29.59% of 256 KB) | 416214 B (39.69% of 1 MB) | 50940 B (19.43% of 256 KB) |
| **samples/sensor/sht3xd** | 26608 B (10.15% of 256 KB) | 46758 B (4.46% of 1 MB) | 1056 B (0.40% of 256 KB) |
| **samples/usb/console** | 33520 B (12.79% of 256 KB) | 49428 B (4.71% of 1 MB) | 4688 B (1.79% of 256 KB) | -->

| Sample | RAMILM | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 23920 B (9.12% of 256 KB) | 31480 B (3.00% of 1 MB) | 1032 B (0.39% of 256 KB) |
| **samples/basic/button** | 23920 B (9.12% of 256 KB) | 32120 B (3.06% of 1 MB) | 1048 B (0.40% of 256 KB) |
| **samples/basic/fade_led** | 25968 B (9.91% of 256 KB) | 45932 B (4.38% of 1 MB) | 2464 B (0.94% of 256 KB) |
| **samples/bluetooth/peripheral_ht** | 54496 B (20.79% of 256 KB) | 206414 B (19.69% of 1 MB) | 8344 B (3.18% of 256 KB) |
| **samples/crypto/mbedtls** | 29040 B (11.08% of 256 KB) | 131676 B (12.56% of 1 MB) | 2620 B (1.00% of 256 KB) |
| **samples/drivers/adc/adc_dt** | 25296 B (9.65% of 256 KB) | 41118 B (3.92% of 1 MB) | 1660 B (0.63% of 256 KB) |
| **samples/drivers/spi_flash** | 24352 B (9.29% of 256 KB) | 39774 B (3.79% of 1 MB) | 1300 B (0.50% of 256 KB) |
| **samples/drivers/watchdog** | 23920 B (9.12% of 256 KB) | 39828 B (3.80% of 1 MB) | 1104 B (0.42% of 256 KB) |
| **samples/hello_world** | 23920 B (9.12% of 256 KB) | 30952 B (2.95% of 1 MB) | 1032 B (0.39% of 256 KB) |
| **samples/net/openthread/cli** | 58064 B (22.15% of 256 KB) | 573736 B (54.72% of 1 MB) | 89216 B (34.03% of 256 KB) |
| **samples/net/sockets/echo_client** | 36140 B (13.79% of 256 KB) | 259894 B (24.79% of 1 MB) | 34640 B (13.21% of 256 KB) |
| **samples/net/sockets/echo_server** | 74860 B (28.56% of 256 KB) | 423586 B (40.40% of 1 MB) | 50964 B (19.44% of 256 KB) |
| **samples/sensor/sht3xd** | 23920 B (9.12% of 256 KB) | 44552 B (4.25% of 1 MB) | 1064 B (0.41% of 256 KB) |

<!-- ---

### TLSR9118BDK40D (tlsr9118bdk40d)

📈 **Resource Usage Details**

| Sample | RAM_ILM | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 60 B (0.18% of 32 KB) | 23732 B (0.57% of 4 MB) | 24776 B (12.60% of 192 KB) |
| **samples/basic/button** | 60 B (0.18% of 32 KB) | 24336 B (0.58% of 4 MB) | 24784 B (12.61% of 192 KB) |
| **samples/bluetooth/peripheral_ht** | 60 B (0.18% of 32 KB) | 137384 B (3.28% of 4 MB) | 39572 B (20.13% of 192 KB) |
| **samples/boards/tlsr9x/gpio-kbd-matrix** | 60 B (0.18% of 32 KB) | 30960 B (0.74% of 4 MB) | 27620 B (14.05% of 192 KB) |
| **samples/boards/tlsr9x/key_matrix** | 60 B (0.18% of 32 KB) | 24472 B (0.58% of 4 MB) | 24856 B (12.64% of 192 KB) |
| **samples/boards/tlsr9x/key_pool** | 60 B (0.18% of 32 KB) | 24420 B (0.58% of 4 MB) | 24848 B (12.64% of 192 KB) |
| **samples/boards/tlsr9x/led_pool** | 60 B (0.18% of 32 KB) | 25020 B (0.60% of 4 MB) | 24840 B (12.63% of 192 KB) |
| **samples/boards/tlsr9x/pwm_pool** | 60 B (0.18% of 32 KB) | 25388 B (0.61% of 4 MB) | 24840 B (12.63% of 192 KB) |
| **samples/boards/tlsr9x/sock_simple** | 60 B (0.18% of 32 KB) | 154144 B (3.68% of 4 MB) | 73096 B (37.18% of 192 KB) |
| **samples/boards/tlsr9x/sock_simple** | 60 B (0.18% of 32 KB) | 154280 B (3.68% of 4 MB) | 73064 B (37.16% of 192 KB) |
| **samples/crypto/mbedtls** | 108 B (0.33% of 32 KB) | 125928 B (3.00% of 4 MB) | 30188 B (15.35% of 192 KB) |
| **samples/drivers/spi_flash_at45** | 60 B (0.18% of 32 KB) | 38524 B (0.92% of 4 MB) | 27064 B (13.77% of 192 KB) |
| **samples/drivers/uart/echo_bot** | 60 B (0.18% of 32 KB) | 26088 B (0.62% of 4 MB) | 26520 B (13.49% of 192 KB) |
| **samples/net/openthread/cli** | 60 B (0.18% of 32 KB) | 549644 B (13.10% of 4 MB) | 143248 B (72.86% of 192 KB) |
| **samples/sensor/mpu6050** | 60 B (0.18% of 32 KB) | 38052 B (0.91% of 4 MB) | 24876 B (12.65% of 192 KB) |
| **samples/subsys/nvs** | 60 B (0.18% of 32 KB) | 38112 B (0.91% of 4 MB) | 24860 B (12.64% of 192 KB) |
| **samples/subsys/shell/devmem_load/** | 60 B (0.18% of 32 KB) | 52400 B (1.25% of 4 MB) | 28888 B (14.69% of 192 KB) |
| **tests/drivers/adc/adc_api** | 60 B (0.18% of 32 KB) | 55976 B (1.33% of 4 MB) | 24548 B (12.49% of 192 KB) | -->

---

### 📝 Additional Notes

- **Memory Regions:** May vary between chip variants; check individual board configurations
- **Full CI Data:** For complete resource usage information across all samples (including Bluetooth, OpenThread, and MCUBoot), refer to CI build artifacts from [PR #789](https://github.com/telink-semi/zephyr/pull/789)
- **Production Optimizations:** For production builds, perform further development, disable debug logging and enable appropriate optimizations to reduce RAM/ROM usage
- **Bluetooth & OpenThread:** For Bluetooth LE and OpenThread-specific resource usage, see the respective CI workflow files in `.github/workflows/`
- **Build Config:** All builds use `-DCONFIG_COMPILER_WARNINGS_AS_ERRORS=y` as in the CI pipelines

---

## 🔗 Resources

Made by Telink Semiconductor

- [Website](https://www.telink-semi.com/)
- [Forum](https://forum.telink-semi.cn/)
- [Documentation](https://doc.telink-semi.cn/)
- [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md)
- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html)