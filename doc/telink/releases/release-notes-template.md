# Telink Zephyr SDK Release Note

[![Version](https://img.shields.io/badge/Version-{{VERSION}}--beta--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/zephyr/releases/tag/{{VERSION}}-beta-v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](LICENSE)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)

---

- **Release Type:** Pre-Release (Beta)
- **Branch:** dev-tlk_v4.1

---

## 📖 Introduction

This release is based on the `dev-tlk_v4.1` branch. _TODO: add a one-sentence summary of key changes._

### Based on Upstream Zephyr v4.1.0

This release is based on upstream Zephyr **v4.1.0** (tag [`v4.1.0`](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)). For the full list of upstream changes, see:

- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html) — recommended reading when upgrading from a previous Zephyr version.

For environment setup, see the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md).

---

## ✨ Highlights

| Category | Details |
|----------|---------|
| **New Chips** | _TODO_ |
| **New Features** | _TODO_ |
| **CI/CD** | _TODO_ |
| **Driver Updates** | _TODO_ |

_TODO: add a brief descriptive paragraph summarizing the key changes of this release._

---

## 🔒 Security

This release includes all security fixes inherited from upstream Zephyr v4.1.0, addressing the following CVEs:

- [CVE-2025-1673](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jjhx-rrh4-j8mx)
- [CVE-2025-1674](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x975-8pgf-qh66)
- [CVE-2025-1675](https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2m84-5hfw-m8v4)

More detailed information can be found in the [Zephyr Security Vulnerabilities](https://docs.zephyrproject.org/latest/security/vulnerabilities.html) page.

---

## 🆕 New Features

- _TODO: list new features of this release_

---

## 🐛 Bug Fixes

| Issue | Component | Description |
|-------|-----------|-------------|
| _TODO_ | _TODO_ | _TODO_ |

---

## 🔧 API & Kconfig Changes

### Telink-Specific Changes

| Change | Impact | Migration |
|--------|--------|-----------|
| _TODO_ | _TODO_ | _TODO_ |

### Upstream Inherited Changes

This release inherits all upstream Zephyr v4.1.0 API and Kconfig changes.

For the complete list, see the upstream [Zephyr v4.1.0 Release Notes — API Changes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html#api-changes) and the [Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html).

---

## 📦 Updates / Dependencies

| Component | Repository | Commit | Notes |
|-----------|------------|--------|-------|
| **Telink BLE SDK** | [telink-semi/tl_ble_sdk_zephyr](https://github.com/telink-semi/tl_ble_sdk_zephyr) | _TODO_ | Required by all Telink SoCs; fetch via `./hal_v2/fetch_sdk.sh` (hal_v2) or `west blobs fetch hal_telink` (hal_v1) |
| **Telink HAL Zephyr** | [telink-semi/hal_telink](https://github.com/telink-semi/hal_telink) | _TODO_ | Contains both hal_v1 and hal_v2 sources |
| **MCUBoot** | [telink-semi/mcuboot](https://github.com/telink-semi/mcuboot) | _TODO_ | Bootloader; required for OTA/DFU |
| **OpenThread Telink** | [telink-semi/openthread](https://github.com/telink-semi/openthread) | _TODO_ | OpenThread source adapted for Telink |
| **OpenThread Telink Lib** | [telink-semi/openthread_telink_lib](https://github.com/telink-semi/openthread_telink_lib) | _TODO_ | Pre-built OpenThread library for Telink |

---

## ⚠️ Known Issues and Limitations

- **WEST tool does not auto-fetch `tl_ble_sdk`:** Zephyr CI does not allow modules containing binary files, so `west update` will not pull `tl_ble_sdk` automatically. See [Important Notes](#-important-notes) for the manual fetch step.
- **TL721X HAL migration (hal_v1 → hal_v2):** TL721X uses **hal_v2** in this release. Fetch the BLE stack via `./hal_v2/fetch_sdk.sh` instead of `west blobs fetch hal_telink`.
- **Toolchain:** Only `riscv64-zephyr-elf` is validated for Telink SoCs. Other toolchains (e.g. experimental IAR) are not tested.
- _TODO: add release-specific known issues_

---

## 📌 Important Notes

- Manually fetch `tl_ble_sdk` by running `./hal_v2/fetch_sdk.sh` inside `modules/hal/telink/` to pull or update the BLE stack to the version pinned by this release.
- For full environment setup instructions, refer to the [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md) or the [Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/).

---

## 📋 Version Information

### Zephyr SDK & Toolchain

| Component | Version |
|-----------|---------|
| **Host OS (recommended)** | Ubuntu 24.04 LTS |
| **Zephyr SDK Version** | Telink Zephyr v4.1.0 |
| **Zephyr SDK** | 0.17.0 |
| **Toolchain** | riscv64-zephyr-elf |

> **Note:** Only the `riscv64-zephyr-elf` toolchain (from Zephyr SDK 0.17.0) is validated for Telink SoCs. The experimental IAR compiler support introduced in upstream Zephyr v4.1.0 is **not** tested with this release. Ubuntu 24.04 LTS is the recommended host OS.

### Telink SDK

| Property | Value |
|----------|-------|
| **Branch** | dev-tlk_v4.1 |
| **Target Commit** | _TODO_ |
| **Tag Name** | {{VERSION}}-beta-v4.1.0 |
| **Release Type** | Pre-Release (Beta) |

### Chip & Hardware Versions

📦 **Chip Versions**

| Chip Family | Versions |
|-------------|----------|
| TLSR921X/TLSR951X(B91) | A2 |
| TLSR922X/TLSR952X(B92) | A3/A4 |
| TL721X | A2/A3 |
| TL321X | A1/A2/A3 |
| TL322X | A1 |
| TL323X | A0 |

🔧 **Hardware EVK Versions**

| Chip | EVK Version |
|------|-------------|
| TLSR921X | C1T213A20_V1.3 |
| TLSR952X | C1T266A20_V1.3 |
| TL721X | C1T315A20_V1.2 |
| TL321X | C1T331A20_V1.0/C1T335A20_V1.3 |
| TL322X | C1T382A20_V1.2 |
| TL323X | C1T388A20_V1.1 |

---

## 📊 Resource Usage (Code Size)

_The content below (Supported Boards, Zephyr Samples Support Matrix, build
environment note, and per-chip-family resource usage tables) is
auto-generated by `build_and_update_notes.py`. Edit the _TODO_ sections and
static content above this line before running the script._

---

## 🔗 Resources

Made by Telink Semiconductor

- [Website](https://www.telink-semi.com/)
- [Forum](https://forum.telink-semi.cn/)
- [Documentation](https://doc.telink-semi.cn/)
- [Telink Zephyr SDK Getting Started Guide](../getting_started/index.md)
- [Zephyr v4.1.0 Release Notes](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)
- [Zephyr v4.1.0 Migration Guide](https://docs.zephyrproject.org/latest/migration/migration-4.1.html)
