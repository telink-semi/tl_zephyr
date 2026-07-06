# Telink Zephyr SDK

[![Telink Website](https://img.shields.io/badge/Website-Telink-blue?style=flat-square)](https://www.telink-semi.com/)
[![Forum](https://img.shields.io/badge/Forum-Telink-green?style=flat-square)](https://forum.telink-semi.cn/)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](LICENSE)

---

## 1. SDK Introduction

Telink Zephyr SDK is a software development platform for Telink RISC-V SoC platforms and is based on Zephyr Project. It is designed for developing a wide range of wireless IoT products including smart home,wearable electronics,lightning products, etc.

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

## 🚀 Quick Reference for Development

For a detailed list of supported devices, please refer to the [Release Note](tl_zephyr_sdk_Release_Note.md).

For development environment setup, SDK acquisition, and quick-start instructions, please refer to the [SDK Get Started](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/#experience-the-implementation-and-function-of-matter).

For detailed software architecture, repository structure, and functional module descriptions, please refer to the [SDK Handbook](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/).

For more sample projects, please refer to the [Standard Zephyr Examples](/zephyr/samples).

---

## 📚 Additional Reference Documents

| Document | Description | Link |
|----------|-------------|------|
| Get Started | SDK Quick Start Guide | [Get Started](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/#experience-the-implementation-and-function-of-matter) |
| Handbook | SDK & Usage Details | [Handbook](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/) |
| Examples | Sample Project Walkthroughs | [Example](/zephyr/samples/basic/blinky) |
| API Reference | API Documentation & Lookup | [API Reference](https://docs.zephyrproject.org/4.1.0/doxygen/html/index.html)(No custom APIs are added in this module; all functions are implemented by directly calling the platform's public APIs.) |
| Release Note | SDK Changelog & New Features | [Release Note](tl_zephyr_sdk_Release_Note.md) |
| Telink Official Forum | Technical Support & Discussion | [Forum - Telink](https://forum.telink-semi.cn/) |
| Telink Official Website | Product Page & Documentation | [Telink - Chips for a Smarter IoT](https://www.telink-semi.com/) |
| Original Zephyr | ZephyrRTOS community README | [ZephyrRTOS community README](README.rst) |

---

## 📝 Release Information

For version history and details changelog,please refer to the [Release Note](tl_zephyr_sdk_Release_Note.md).

---

## Contribution Guide

For contribution guidelines, please refer to the [Contribution Guide](CONTRIBUTING.rst).

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