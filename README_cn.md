# SDK 介绍

* [英文 README](README.md)

Telink Zephyr SDK 是基于 Zephyr Project 构建的面向 Telink RISC-V SoC 平台的软件开发平台。通过 Telink Zephyr SDK，开发者可以基于 Zephyr RTOS 标准开发框架，在 Telink SoC 平台上快速开发嵌入式应用。

该 SDK 在 Zephyr RTOS 开源生态基础上，提供 Telink SoC 平台适配支持，包括：

- SoC 芯片支持
- 开发板支持
- 硬件驱动支持
- Device Tree 配置
- 构建系统配置
- 参考示例工程

**SDK 核心能力**

| 类别 | 能力说明 |
| --- | --- |
| 芯片平台支持 | Telink RISC-V SoC 支持、启动配置、内存及链接配置、开发板支持 |
| 硬件驱动支持 | 基于 Zephyr Driver Model 的 GPIO、UART、SPI、I²C、PWM、ADC、Timer、Flash 等外设支持 |
| 软件框架支持 | Zephyr Kernel、Device Model、Driver Framework、Kconfig、West 构建系统 |
| 系统服务 | 电源管理、日志系统、存储管理、系统配置 |
| 安全能力 | MCUboot、安全启动、加密框架支持 |
| 生态支持 | 基于 Zephyr 生态支持 Bluetooth® LE、Thread、Matter 等应用开发 |
| 示例支持 | Zephyr Samples 及 Telink reference examples |

Telink Zephyr SDK 在保持与 Zephyr 生态兼容的基础上，增加了 Telink SoC 支持包、无线连接能力及参考示例，帮助开发者快速完成产品开发。

开发者可基于本 SDK，利用 Zephyr RTOS 生态和 Telink SoC 硬件能力，开发以下类型的嵌入式无线物联网产品：

- Bluetooth® LE 设备（HID、传感器、Beacon、可穿戴设备等）
- Thread 智能家居设备（智能照明、传感器、控制设备等）
- Matter 智能家居设备（灯具、插座、门锁、温控器等）
- 低功耗 IoT 终端设备（环境监测、资产管理、工业传感节点等）
- 消费电子及复杂嵌入式设备（无线外设、智能控制器等）

**支持信息**

关于完整、准确的芯片型号、对应的开发板、开发平台、工具链以及 SDK 版本的详细信息，请参阅 [Release Notes](http://./doc/telink/releases/release-notes-tl_v1.0.1.md)。打开 Release Notes 页面后，通过左侧下拉列表，选择与您当前使用的 SDK 版本对应的 Release Notes 查看。

# 文档与资源

**文档导航**

| 文档 | 说明 |
| --- | --- |
| 快速入门 | 开发环境配置、SDK获取及快速上手方法 |
| [Telink Matter 开发手册](https://doc.telink-semi.cn/doc/zh/software/res/sdk/matter/telink_matter_developer_guide_cn/) | Telink Matter SDK 的软件架构、仓库结构及功能模块说明 |
| Release Notes | 支持平台、版本说明及详细变化 |

**社区与资源**

| 资源 | 说明 |
| --- | --- |
| [Telink 官方论坛](https://forum.telink-semi.cn/) | 技术交流与支持 |
| [Telink 官方网站](https://www.telink-semi.com/) | 产品中心及文档中心 |
| [Zephyr Project](https://github.com/zephyrproject-rtos/zephyr/blob/v4.1-branch/README.rst) | Zephyr 官方社区 |
| [Zephyr Project Documentation](https://docs.zephyrproject.org/latest/) | Zephyr 技术文档 |
| [Zephyr 示例工程](https://docs.zephyrproject.org/latest/samples/index.html#samples) | Zephyr 示例程序说明 |
| [API Reference](https://docs.zephyrproject.org/4.1.0/doxygen/html/index.html) | API 查询 |
| [GitHub](https://github.com/telink-semi/zephyr)  | SDK 源码仓库 |

# 贡献指南

欢迎开发者参与 Telink Zephyr SDK 的持续改进。

提交 Issue、贡献代码及开发规范，请参阅 [Contribution Guide](https://docs.zephyrproject.org/latest/contribute/index.html)。

# 许可证

本项目采用以下许可证：

**Apache License, Version 2.0**

Licensed under the Apache License, Version 2.0 (the "License");

You may not use this file except in compliance with the License.

You may obtain a copy of the License at:

[http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0)

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

See the License for the specific language governing permissions and limitations under the License.
