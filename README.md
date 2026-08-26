# Telink Zephyr SDK

[![Telink Website](https://img.shields.io/badge/Website-Telink-blue?style=flat-square)](https://www.telink-semi.com/)
[![Forum](https://img.shields.io/badge/Forum-Telink-green?style=flat-square)](https://forum.telink-semi.cn/)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](LICENSE)

---

**A software development platform for Telink RISC-V SoC platforms based on Zephyr Project**

- For development environment setup, SDK acquisition, and quick-start instructions, refer to SDK [Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/).
- Specifically, check Chapter **Install Zephyr Project Environment** as the initial version of Getting Started in the [Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/#experience-the-implementation-and-function-of-matter).
- For a detailed list of supported devices, refer to the [Release Note](tl_zephyr_sdk_Release_Note.md)
- For community resources and support, refer to [Zephyr RTOS Community README](README.rst)

---

## 📖 SDK Introduction

Telink Zephyr SDK is a software development platform for Telink RISC-V SoC platforms and is based on Zephyr Project.
It is designed for developing a wide range of wireless IoT products including smart home, wearables, lighting products, etc.

### What's Included

- ✅ Zephyr RTOS kernel
- ✅ Telink Hardware Abstraction Layer
- ✅ Telink wireless connectivity stack
- ✅ MCUboot bootloader
- ✅ Power management framework
- ✅ Sample applications and board configurations

### Supported Products

| Product Type | Description |
|--------------|-------------|
| **Bluetooth LE** | Wireless sensors, beacons, HID devices, audio streaming, wearables |
| **Thread / Matter** | Lights, switches, sensors, door locks, and Matter over Thread devices |
| **Wi-Fi enabled IoT devices** | Gateways, Lights, and cloud-connected edge devices |

---

## 🚀 Quick Reference

| Resource | Description | Link |
|----------|-------------|------|
| **Release Note** | Telink SDK Changelog & New Features | [Telink Zephyr SDK Release Note](tl_zephyr_sdk_Release_Note.md) |
| **Get Started Guide/Handbook** | SDK Quick Start Guide & Developer Handbook | Check Chapter **Install Zephyr Project Environment** in the [Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/#experience-the-implementation-and-function-of-matter) |
| **Examples** | Sample Project Walkthroughs | See [Standard Zephyr Samples](samples) |
| **API Reference** | Zephyr API Documentation & Lookup  | See [Zephyr API Docs](https://docs.zephyrproject.org/4.1.0/doxygen/html/index.html) |

---

## 📚 Additional Resources

| Type | Resource |
|------|----------|
| 🌐 **Official Website** | [Telink - Chips for a Smarter IoT](https://www.telink-semi.com/) |
| 💬 **Forum** | [Telink Technical Support](https://forum.telink-semi.cn/) |
| 📖 **Documentation** | [Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/) |
| 📦 **Original Zephyr** | [Zephyr RTOS Community README](README.rst) |

---

## 🧩 Telink Maintained Repositories

Besides this repository, the SDK pulls in the following Telink maintained
modules through `submanifests/telink.yaml`. They carry Telink specific
modifications and optimizations on top of their upstream projects, or are
Telink provided components, and are therefore published under the `tl_`
naming convention:

| Repository | Upstream | Telink Modifications |
|------------|----------|----------------------|
| [tl_mcuboot](https://github.com/telink-semi/tl_mcuboot) | [MCUboot](https://github.com/mcu-tools/mcuboot) | Boot time optimizations: the primary slot image is validated only on the first boot (`BOOT_VALIDATE_SLOT0_ONCE`); on TL323X the application image hash is computed directly from flash storage (`CONFIG_BOOT_IMG_HASH_DIRECTLY_ON_STORAGE`) without a RAM copy. |
| [tl_openthread](https://github.com/telink-semi/tl_openthread) | [OpenThread](https://github.com/openthread/openthread) | Performance optimization: the sleepy end device (SED) processing call tree (tasklets, timers, MAC frame handling, mesh forwarding) is placed in the RAM code section (`OT_SED_RAM`) for faster execution on Telink SoCs. |
| [tl_xz](https://github.com/telink-semi/tl_xz) | [XZ Utils](https://github.com/tukaani-project/xz) | Zephyr module integration for `liblzma` (a `zephyr/` CMake and Kconfig wrapper) with size optimization (`-Os`) enabled by default. |
| [tl_openthread_libs](https://github.com/telink-semi/tl_openthread_libs) | Telink owned | Prebuilt OpenThread FTD libraries for Telink SoCs, in extended and reduced feature set variants, linked as a Zephyr module when `CONFIG_OPENTHREAD_TELINK_LIBRARY` is selected. |

The Telink HAL module [hal_telink](https://github.com/telink-semi/hal_telink) keeps the
upstream Zephyr HAL naming convention (`hal_<vendor>`) and is fetched through the
top-level `west.yml`; the NFC driver module `nfc_st25dv` keeps its component name as
well. All other manifest projects are consumed unchanged from their upstream
repositories.

---

## 📝 Release Information

For version history and detailed changelog, refer to the [Release Note](tl_zephyr_sdk_Release_Note.md).

---

## Contribution Guide

For contribution guidelines, refer to the [Contribution Guide](CONTRIBUTING.md).

---

## 📄 License

```
Apache License, Version 2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

---

Made by Telink Semiconductor
