# Telink Zephyr SDK Release Note

[![Version](https://img.shields.io/badge/Version-tl_v1.0.1--rc1--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/zephyr/releases/tag/tl_v1.0.1-rc1-v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](LICENSE)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)

---

- **Release Type:** Pre-Release (rc1)
- **Branch:** release-v1.0-v4.1-branch
- **Tag Version:** tl_v1.0.1-rc1-v4.1.0
<!-- - **Target Commit:** e08fc42546e58d808bfd39f35c8df296f5617a44 -->

---

## 📖 Introduction

This release is based on the latest commit of `release-v1.0-v4.1-branch` branch, incorporating multiple bug fixes, driver updates, and BLE SDK improvements for Telink TL323x series chips and other platforms.

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
| **Branch** | release-v1.0-v4.1-branch |
| **Tag Name** | tl_v1.0.1-rc1-v4.1.0 |
| **Release Type** | Pre-Release (rc1) |

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
| TL721X | C1T315A20_V1.2 |
| TL323X | C1T388A20_V1.1 |

---

## ✨ Highlights

<!-- | Category | Details |
|----------|---------|
| **New Chips** | Full support for tl323x series |
| **New Features** | LZMA configuration support, TL523X skeleton board |
| **CI/CD** | Dedicated pipelines for tl323x platform |
| **Driver Updates** | PLIC, pinctrl, SHA HW cryptography | -->

| Category | Details |
|----------|---------|
| **New Chips** | Full support for tl323x series |
| **CI/CD** | Dedicated pipelines for tl323x platform |
| **Driver Updates** | PLIC, pinctrl, SHA HW cryptography |

This release introduces full support for the **TL323X** series — a new
RISC-V based Telink SoC — including device tree, pin configuration and a
dedicated CI pipeline.
**LZMA** compression configuration is now available
for Telink SoCs, enabling smaller OTA images that fit 2 MB flash.

The TL721X platform has been **migrated from hal_v1 to hal_v2**, with
optimized power consumption; existing TL721X users should follow the
migration note in [Known Issues](#-known-issues-and-limitations).
Driver
work includes a new PLIC interrupt controller, pinctrl fixes, and SHA
hardware-accelerated cryptography on TLX platforms.

---

## 🔒 Security

This release includes all security fixes inherited from upstream Zephyr v4.1.0, addressing the following CVEs:

- [CVE-2025-1673](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jjhx-rrh4-j8mx)
- [CVE-2025-1674](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x975-8pgf-qh66)
- [CVE-2025-1675](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2m84-5hfw-m8v4)

More detailed information can be found in the [Zephyr Security Vulnerabilities](https://docs.zephyrproject.org/latest/security/vulnerabilities.html) page.

---

## 🆕 New Features

<!-- - ✅ Added full support for tl323x series chips
- ✅ New CI build pipelines specifically for the tl323x platform
- ✅ Added device tree and pin configuration support for the tl3238x development board
- ✅ Added PLIC interrupt controller support
- ✅ Added LZMA module configuration support for Telink SoCs
- ✅ Added tl523x skeleton board support -->

- ✅ Added full support for tl323x series chips
- ✅ New CI build pipelines specifically for the tl323x platform
- ✅ Added device tree and pin configuration support for the tl3238x development board
- ✅ Added PLIC interrupt controller support
- ✅ Added LZMA module configuration support for Telink SoCs

---

## 🐛 Bug Fixes

<!-- | Issue | Component | Description |
|-------|-----------|-------------|
| **TL323x RF TX** | RF | Resolves 1M PHY DEVM transmission performance issue on TL323x series |
| **WFI Function** | Kernel/Arch | Adjusted macro definitions and removed `ARCH_HAS_CUSTOM_CPU_IDLE` |
| **Amazon Issues** | RF/SOC | Reset RF related registers in `soc_early_init_hook` to fix jump/reconnect failures |
| **PM/Clock** | PM | Updated reset and clock clear on TL323x PM |
| **AES Reentrancy** | Crypto | Resolved AES reentrancy issue |
| **Pinctrl** | Driver | Fixed peripherals input pins; fixed pinctrl Kconfig to always disable GPIOs |
| **PWM Driver** | Driver | Reverted incorrect PWM driver changes for Telink platform |
| **SHA HW Crypto** | Crypto | Reworked Telink SHA calculation using HW unit on TLX platforms |
| **Kconfig** | Build | Fixed dependency for bootloader HW cryptography on Telink B9X & TLX platforms |
| **32K Watchdog** | PM | Changed logic - open 32k wd in idle/standby mode |
| **TL721X V2** | HAL | Updated hal_v1 to hal_v2, optimized power consumption | -->

| Issue | Component | Description |
|-------|-----------|-------------|
| **TL323x RF TX** | RF | Resolves 1M PHY DEVM transmission performance issue on TL323x series |
| **WFI Function** | Kernel/Arch | Adjusted macro definitions and removed `ARCH_HAS_CUSTOM_CPU_IDLE` |
| **Amazon Issues** | RF/SOC | Reset RF related registers in `soc_early_init_hook` to fix jump/reconnect failures |
| **PM/Clock** | PM | Updated reset and clock clear on TL323x PM |
| **AES Reentrancy** | Crypto | Resolved AES reentrancy issue |
| **Pinctrl** | Driver | Fixed peripherals input pins; fixed pinctrl Kconfig to always disable GPIOs |
| **PWM Driver** | Driver | Reverted incorrect PWM driver changes for Telink platform |
| **SHA HW Crypto** | Crypto | Reworked Telink SHA calculation using HW unit on TLX platforms |
| **Kconfig** | Build | Fixed dependency for bootloader HW cryptography on Telink TLX platforms |
| **32K Watchdog** | PM | Changed logic - open 32k wd in idle/standby mode |
| **TL721X V2** | HAL | Updated hal_v1 to hal_v2, optimized power consumption |

---

## 🔧 API & Kconfig Changes

### Telink-Specific Changes

The following Telink-specific Kconfig changes may affect existing applications when upgrading to this release:

<!-- | Change | Impact | Migration |
|--------|--------|-----------|
| **`ARCH_HAS_CUSTOM_CPU_IDLE` removed** | WFI handling is now driven by the standard Zephyr path | Remove any custom override of `ARCH_HAS_CUSTOM_CPU_IDLE`; rely on the default `cpu_idle` implementation |
| **Bootloader HW cryptography dependency** | Kconfig dependency for bootloader HW cryptography on Telink B9X & TLX platforms was fixed | Re-check your bootloader crypto Kconfig selection if you use HW crypto on B9X/TLX |
| **PLIC interrupt controller** | New interrupt controller driver added for TL323X | No migration needed for new projects; existing TL323X boards now use PLIC by default |
| **Pinctrl Kconfig** | Pinctrl Kconfig now always disables GPIOs on conflicting pins | Verify your DTS pin assignments if you relied on the old dual-use behavior | -->

| Change | Impact | Migration |
|--------|--------|-----------|
| **`ARCH_HAS_CUSTOM_CPU_IDLE` removed** | WFI handling is now driven by the standard Zephyr path | Remove any custom override of `ARCH_HAS_CUSTOM_CPU_IDLE`; rely on the default `cpu_idle` implementation |
| **Bootloader HW cryptography dependency** | Kconfig dependency for bootloader HW cryptography on Telink TLX platforms was fixed | Re-check your bootloader crypto Kconfig selection if you use HW crypto on TLX |
| **PLIC interrupt controller** | New interrupt controller driver added for TL323X | No migration needed for new projects; existing TL323X boards now use PLIC by default |
| **Pinctrl Kconfig** | Pinctrl Kconfig now always disables GPIOs on conflicting pins | Verify your DTS pin assignments if you relied on the old dual-use behavior |

### Upstream Inherited Changes

This release inherits all upstream Zephyr v4.1.0 API and Kconfig changes.

For the complete list, see the upstream [Zephyr v4.1.0 Release Notes — API Changes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html#api-changes) and the [Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html).

---

## 📦 Updates / Dependencies

| Component | Repository | Commit | Notes |
|-----------|------------|--------|-------|
| **Telink BLE SDK** | [telink-semi/tl_ble_sdk_zephyr](https://github.com/telink-semi/tl_ble_sdk_zephyr) | [`46322e5`](https://github.com/telink-semi/tl_ble_sdk_zephyr/commit/46322e5b570e2a68373b18d4f08811acadd1266c) | Required by all Telink SoCs; fetch via `./hal_v2/fetch_sdk.sh` (hal_v2) or `west blobs fetch hal_telink` (hal_v1) |
| **Telink HAL Zephyr** | [telink-semi/hal_telink](https://github.com/telink-semi/hal_telink) | [`14c6149`](https://github.com/telink-semi/hal_telink/commit/14c6149f6cc466c49d81e3b2f7f1e4d8ff6fbbb5) | Contains both hal_v1 and hal_v2 sources |
| **MCUBoot** | [telink-semi/mcuboot](https://github.com/telink-semi/mcuboot) | [`ce0da85`](https://github.com/telink-semi/mcuboot/commit/ce0da85c39c749df49b0ec62b33d2ecdea24c927) | Bootloader; required for OTA/DFU |
| **OpenThread Telink** | [telink-semi/openthread](https://github.com/telink-semi/openthread) | [`542aaab`](https://github.com/telink-semi/openthread/commit/542aaab44e1308e1a8a24573dfbd413fade342ee) | OpenThread source adapted for Telink |
| **OpenThread Telink Lib** | [telink-semi/openthread_telink_lib](https://github.com/telink-semi/openthread_telink_lib) | [`308dae2`](https://github.com/telink-semi/openthread_telink_lib/commit/308dae2f80084f87073cfd4fbd30f1be0799be7b) | Pre-built OpenThread library for Telink |

---

## ⚠️ Known Issues and Limitations

- **Pre-release:** This is a pre-release version for demonstration, development and testing purposes. Not recommended for production use.
- **WEST tool does not auto-fetch `tl_ble_sdk`:** Zephyr CI does not allow modules containing binary files, so `west update` will not pull `tl_ble_sdk` automatically. See the [Important Notes](#-important-notes) below for the manual fetch step.
- **TL721X HAL migration (hal_v1 → hal_v2):** TL721X has been moved from **hal_v1** to **hal_v2** in this release. If you are upgrading from a previous version where TL721X used `west blobs fetch hal_telink` (hal_v1), you must now fetch the BLE stack via `./hal_v2/fetch_sdk.sh` instead. The build will fail with a missing HAL error if you forget this step.
<!-- - **Toolchain:** Only the `riscv64-zephyr-elf` toolchain is validated for Telink SoCs. Other toolchains (e.g. the experimental IAR support introduced in upstream v4.1.0) are not tested with this release. -->

---

## 📌 Important Notes

- Manually fetch `tl_ble_sdk` by running `./hal_v2/fetch_sdk.sh` inside `modules/hal/telink/` to pull or update the BLE stack to the version pinned by this release.
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

---

### Zephyr Samples Support Matrix

The table below summarizes build and test status for Zephyr samples across Telink
chip families in this release. Samples are located under `samples/` in the Zephyr
tree (tests are under `tests/`).

> ✅ = Supported and Tested &nbsp;&nbsp;
> 🟡 = Supported but Untested
> &nbsp;&nbsp; (builds successfully, not functionally validated)
<!-- > &nbsp;&nbsp; · = Untested (not built or not applicable) -->

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

| Sample | TL323X | TL721X |
|--------|:------------:|:------------:|
| **basic/blinky** | ✅ | ✅ |
| **basic/button** | ✅ | ✅ |
| **basic/fade_led** | ✅ | ✅ |
| **hello_world** | ✅ | ✅ |
| **bluetooth/peripheral_ht** | ✅ | ✅ |
| **net/openthread/cli** | ✅ | ✅ |
| **net/sockets/echo_client** | ✅ | ✅ |
| **crypto/mbedtls** | ✅ | ✅ |
| **drivers/adc/adc_dt** | ✅ | 🟡 |
| **drivers/spi_flash** | ✅ | 🟡 |
| **drivers/watchdog** | ✅ | ✅ |
| **sensor/sht3xd** | ✅ | 🟡 |

#### Notes on Sample Support

<!-- - **Tested combinations (✅):** All TL323X samples that build successfully have been
  functionally validated. On TL721X, the core bring-up samples (`blinky`, `button`,
  `fade_led`, `hello_world`) plus BLE (`peripheral_ht`), Thread (`openthread/cli`),
  crypto (`mbedtls`), networking (`echo_client`), and driver (`watchdog`) samples
  have been functionally validated in this release.
- **Supported but untested (🟡):** All other build targets listed in the table
  compile successfully but have not been functionally validated. Use with caution.
- **Untested (·):** Combinations marked · are not built in this release, either
  because the sample is not applicable to that chip family or because it has
  not been ported yet.
- **Legacy platforms (B91/B92/TL321X/TL322X/W91):** Samples marked 🟡 compile in
  CI but functional testing in this release focused on the new TL323X platform
  and the TL721X hal_v2 migration. Refer to earlier release notes for validated
  sample sets on these platforms. -->

- **Tested combinations (✅):** All TL323X samples that build successfully have been
  functionally validated. On TL721X, the core bring-up samples (`blinky`, `button`,
  `fade_led`, `hello_world`) plus BLE (`peripheral_ht`), Thread (`openthread/cli`),
  crypto (`mbedtls`), networking (`echo_client`), and driver (`watchdog`) samples
  have been functionally validated in this release.
- **Supported but untested (🟡):** All other build targets listed in the table
  compile successfully but have not been functionally validated. Use with caution.
<!-- - **Legacy platforms (B91/B92/TL321X/TL322X/W91):** Samples marked 🟡 compile in
  CI but functional testing in this release focused on the new TL323X platform
  and the TL721X hal_v2 migration. Refer to earlier release notes for validated
  sample sets on these platforms. -->


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

### TL323X (tl3238x)

📈 **Resource Usage Details**

| Sample | RAM_ILM_N | ROM | RAM |
|--------|-----|-----|-----|
| **samples/basic/blinky** | 10060 B (15.35% of 64 KB) | 37792 B (3.60% of 1 MB) | 19528 B (19.86% of 96 KB) |
| **samples/basic/button** | 10060 B (15.35% of 64 KB) | 38424 B (3.66% of 1 MB) | 19544 B (19.88% of 96 KB) |
| **samples/basic/fade_led** | 10060 B (15.35% of 64 KB) | 51256 B (4.89% of 1 MB) | 23004 B (23.40% of 96 KB) |
| **samples/bluetooth/peripheral_ht** | 20740 B (31.65% of 64 KB) | 211240 B (20.15% of 1 MB) | 42464 B (43.20% of 96 KB) |
| **samples/crypto/mbedtls** | 10060 B (15.35% of 64 KB) | 137560 B (13.12% of 1 MB) | 26244 B (26.70% of 96 KB) |
| **samples/drivers/adc/adc_dt** | 10060 B (15.35% of 64 KB) | 52124 B (4.97% of 1 MB) | 20520 B (20.87% of 96 KB) |
| **samples/drivers/spi_flash** | 10228 B (15.61% of 64 KB) | 45004 B (4.29% of 1 MB) | 19676 B (20.02% of 96 KB) |
| **samples/drivers/watchdog** | 10060 B (15.35% of 64 KB) | 45668 B (4.36% of 1 MB) | 19604 B (19.94% of 96 KB) |
| **samples/hello_world** | 10060 B (15.35% of 64 KB) | 37316 B (3.56% of 1 MB) | 19528 B (19.86% of 96 KB) |
| **samples/net/openthread/cli** | 14154 B (21.60% of 64 KB) | 436126 B (41.59% of 1 MB) | 80752 B (82.15% of 96 KB) |
| **samples/net/sockets/echo_client** | 21710 B (33.13% of 64 KB) | 268780 B (25.63% of 1 MB) | 59352 B (60.38% of 96 KB) |
| **samples/sensor/sht3xd** | 10060 B (15.35% of 64 KB) | 50 KB (4.88% of 1 MB) | 19592 B (19.93% of 96 KB) |

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
| **samples/sensor/sht3xd** | 26608 B (10.15% of 256 KB) | 46758 B (4.46% of 1 MB) | 1056 B (0.40% of 256 KB) |

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
- **Full CI Data:** For complete resource usage information across all samples (including Bluetooth, OpenThread, and MCUBoot), refer to CI build artifacts from [PR #774](https://github.com/telink-semi/zephyr/pull/774)
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