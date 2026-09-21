# Telink Zephyr SDK Getting Started Guide

[![Version](https://img.shields.io/badge/Version-tl_v1.4.1--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/tl_zephyr/releases/tag/tl_v1.4.1-v4.1.0)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](../../../LICENSE)

---

## Overview

### About this document

This document helps you quickly set up the Telink Zephyr SDK development
environment on Ubuntu, obtain the SDK, install the build toolchain, configure
the development environment, build a sample, and verify that everything works.

It is intended for developers using the Telink Zephyr SDK for the first time.

By following this document, you will:

1. Set up the Zephyr development environment;
2. Obtain the Telink Zephyr SDK;
3. Install the build toolchain;
4. Configure the development environment;
5. Build a sample;
6. Verify that the development environment works correctly.

After completing this document, you can continue with Bluetooth® LE, Thread,
Matter, and other wireless IoT application development based on the Telink
Zephyr SDK.

### Applicable scope

This guide is the Telink-flavoured counterpart of the upstream
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
The main differences are:

- the Telink fork repository/branch;
- the Telink HAL binary blob fetch step;
- the RISC-V toolchain selection.

For the full walkthrough (with screenshots and detailed tips), see Chapter
*Install Zephyr Project Environment* in the
[Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/).

For detailed information on complete and accurate chip series, corresponding
development boards, development platforms, toolchains, and SDK versions, refer
to the latest [Release Notes](../releases/release-notes.md).

---

## Preparation

### Hardware checklist

| Hardware          | Description                                                                                          |
| ----------------- | ---------------------------------------------------------------------------------------------------- |
| PC                | Linux distribution, Ubuntu 24.04 LTS recommended                                                     |
| Development board | Select a suitable development board according to the Release Notes, e.g. the TL323X Evaluation Kit |
| Programmer        | Telink Programmer V5                                                                                 |
| USB cable         | Connects the PC and the programmer                                                                   |
| Dupont wires      | Connect the programmer and the development board                                                     |

For physical hardware, refer to [Connect the hardware](#step-1-connect-the-hardware).

### Software checklist

| Software            | Description                          |
| ------------------- | ------------------------------------ |
| Programmer software | Burning and Debugging Tool (BDT)     |
| Toolchain           | `riscv64-zephyr-elf`                 |
| SDK                 | [Telink Zephyr SDK](https://github.com/telink-semi/tl_zephyr) |

---

## Set up the development environment

This section uses Ubuntu 24.04 LTS as an example.

### Update and upgrade APT

```bash
sudo apt update
sudo apt upgrade
```

### Install development tools

Install CMake, Python, Device Tree Compiler, and other tools required by the
Zephyr build environment.

The current minimum required versions for the main dependencies are:

| Tool                                              | Min. Version |
| ------------------------------------------------- | ------------ |
| CMake                                             | 3.20.0       |
| Python                                            | 3.6          |
| Devicetree compiler                               | 1.4.6        |

1. Install the Kitware APT repository to obtain a recent CMake, then install
   the required dependencies:

   ```bash
   wget https://apt.kitware.com/kitware-archive.sh
   sudo bash kitware-archive.sh
   ```

   ```bash
   sudo apt install --no-install-recommends git cmake ninja-build gperf \
     ccache dfu-util device-tree-compiler \
     python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
     make gcc gcc-multilib g++-multilib libsdl2-dev
   ```

   > **Note**
   > AArch64 (ARM64) systems do not provide the `gcc-multilib` and
   > `g++-multilib` packages. On an ARM64 host, omit those two packages from
   > the list.

2. Verify the versions of the main dependencies installed on your system:

   ```bash
   cmake --version
   python3 --version
   dtc --version
   ```

   Check those against the table above.

### Install West

[West](https://docs.zephyrproject.org/latest/develop/west/index.html#west) is
Zephyr's multi-repository meta-tool.

1. Create a Python virtual environment:

   ```bash
   python3 -m venv ~/zephyrproject/.venv
   ```

2. Activate the virtual environment:

   ```bash
   source ~/zephyrproject/.venv/bin/activate
   ```

   Once activated, the shell is prefixed with `(.venv)`. The virtual
   environment can be deactivated at any time by running `deactivate`.

   > **Note**
   > Activate the virtual environment every time you start a new terminal
   > session before working with Zephyr. If you do not, commands such as `west`
   > will not be found, or may run against a different Python environment.

3. Install West:

   ```bash
   pip3 install west
   ```

---

## Get the Telink Zephyr SDK

The Telink Zephyr SDK is a fork of the upstream
[zephyrproject-rtos/zephyr](https://github.com/zephyrproject-rtos/zephyr)
repository, hosted at [telink-semi/tl_zephyr](https://github.com/telink-semi/tl_zephyr),
and adapted and extended for Telink RISC-V SoC platforms. The Telink-specific
changes live on the `dev-tlk_v4.1` branch.

### Initialize the Telink Zephyr SDK

1. Initialize the west workspace with the upstream manifest. This creates the
   `~/zephyrproject` workspace and clones the upstream Zephyr repository as the
   manifest repository:

   ```bash
   west init ~/zephyrproject
   cd ~/zephyrproject
   west update
   ```

   > **Tip — China mainland network**
   > Running `west init` and `west update` to fetch the Zephyr source may take
   > extra time in China mainland, and some projects may fail to update from
   > foreign servers. Use a mirror or proxy, or download the source bundle
   > separately.

2. Export the Zephyr CMake package:

   ```bash
   west zephyr-export
   ```

3. Install Zephyr's Python dependencies:

   ```bash
   pip3 install -r ~/zephyrproject/zephyr/scripts/requirements.txt
   ```

4. Add the Telink remote and check out the Telink `dev-tlk_v4.1` branch:

   ```bash
   cd ~/zephyrproject/zephyr
   git remote add telink https://github.com/telink-semi/tl_zephyr
   git fetch telink
   git checkout dev-tlk_v4.1
   cd ..
   west update
   ```

### Fetch the Telink HAL

The Telink Hardware Abstraction Layer (HAL) provides the BLE protocol stack as
a binary blob. The fetch method depends on the chip family.

| HAL version | Supported chips                            | Fetch method                                                                                         |
| ----------- | ------------------------------------------ | ---------------------------------------------------------------------------------------------------- |
| hal_v1      | TLSR9518ADK80D, TLSR9528A, TL321X, TLSR9118 | Run `west blobs fetch hal_telink`. Re-run whenever you switch branches or update the workspace.       |
| hal_v2      | TL322X, TL323X, TL721X                      | `cd ~/zephyrproject/modules/hal/telink/hal_v2`, then `chmod +x fetch_sdk.sh`, then `./fetch_sdk.sh` |

This guide uses the **TL323X**, so use **hal_v2**:

```bash
cd ~/zephyrproject/modules/hal/telink/hal_v2
chmod +x fetch_sdk.sh
./fetch_sdk.sh
```

> **Note**
> `fetch_sdk.sh` uses a default repository (`telink-semi/tl_ble_sdk_zephyr`)
> and the latest commit. To pin a specific repository/commit, pass them as
> arguments:
>
> ```bash
> ./fetch_sdk.sh https://github.com/telink-semi/tl_ble_sdk_zephyr.git de6f125c8ae6d29c4a640ab3526b40ff8c3f0a20
> ```

### Switch branches or commits

If you have completed the setup above and want to switch to a different Telink
branch or commit, run:

```bash
cd ~/zephyrproject/zephyr
git fetch telink
git checkout <your-target-branch-or-commit>
cd ..
west update
```

Then re-fetch the HAL as described in
[Fetch the Telink HAL](#fetch-the-telink-hal), choosing the version that
matches your chip.

### Install the Zephyr SDK toolchain

The Telink Zephyr SDK uses the `riscv64-zephyr-elf` toolchain.

1. Download the Zephyr SDK v0.17.0 minimal archive:

   ```bash
   wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz
   ```

2. Verify the download:

   ```bash
   wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/sha256.sum | shasum --check --ignore-missing
   ```

3. Extract the archive to a recommended path (for example `~/zephyr-sdk-0.17.0`):

   ```bash
   tar -xvf zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz -C ~/
   ```

   The SDK may be installed in any of the following recommended paths:

   ```text
   $HOME/zephyr-sdk[-x.y.z]
   $HOME/.local/zephyr-sdk[-x.y.z]
   $HOME/.local/opt/zephyr-sdk[-x.y.z]
   $HOME/bin/zephyr-sdk[-x.y.z]
   /opt/zephyr-sdk[-x.y.z]
   /usr/zephyr-sdk[-x.y.z]
   /usr/local/zephyr-sdk[-x.y.z]
   ```

   `[-x.y.z]` is the downloaded SDK version, e.g. `-0.17.0`.

   > **Warning**
   > Do **not** move the SDK folder after installation.

4. Install the RISC-V toolchain:

   ```bash
   cd zephyr-sdk-0.17.0
   ./setup.sh -t riscv64-zephyr-elf -h -c
   ```

> **Note — Full SDK**
> If you need the full Zephyr SDK (with host tools such as QEMU and OpenOCD),
> replace the minimal archive with the full one:
>
> ```bash
> wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64.tar.xz
> wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/sha256.sum | shasum --check --ignore-missing
> tar xvf zephyr-sdk-0.17.0_linux-x86_64.tar.xz -C ~/
> cd zephyr-sdk-0.17.0
> ./setup.sh -t riscv64-zephyr-elf -h -c
> ```

### Configure Zephyr environment variables

Append the Zephyr environment script to `~/.bashrc` so that `west build` can
find the workspace in every new terminal:

```bash
echo "source ~/zephyrproject/zephyr/zephyr-env.sh" >> ~/.bashrc
source ~/.bashrc
```

> **Warning**
> If you skip this step, you may encounter `west build` errors later.

---

## Build and flash the first example

Before starting formal Telink Zephyr application development, build and run the
Blinky sample first to verify that the development environment is configured
correctly. This example uses the TL323X EVK.

### Build the first example

1. Navigate to the Zephyr root directory:

   ```bash
   cd ~/zephyrproject/zephyr
   ```

2. Build the Blinky sample:

   ```bash
   west build -p auto -b tl3238x samples/basic/blinky -d build_blinky
   ```

3. Check the build result. After a successful build, you will find `zephyr.bin`
   in the `build_blinky/zephyr/` folder.

### Flash the firmware

BDT is the official programmer for Telink SoCs. It is available for both
Windows and Linux. This document uses Linux BDT (TGui-BDT) with the TL323X.

#### Step 1: Connect the hardware

Connect the PC, the programmer, and the target board as follows:

- **PC ↔ Programmer**: Connect them using a Mini-USB cable. If the green
  indicator LED on the programmer stays solid, the programmer has been
  recognized by the PC.
- **Programmer ↔ Target board**: Connect them using Dupont wires:
  - Power lines: 3V3C ↔ 3V3; GND ↔ GND.
  - Data line (single-wire SWM bus): Connect the programmer's SWM pin to the
    target board's SWS (Swire) pin.

![Connect the device in TGui](figures/tgui_connect.png)

#### Step 2: Download BDT

1. Install the BDT dependencies:

   ```bash
   sudo apt update
   sudo apt install -y libgtk-3-dev libusb-1.0-0-dev
   ```

2. Create a directory for the BDT tools:

   ```bash
   mkdir -p ~/tools/telink-bdt
   ```

3. Download [BDT_Linux.zip](https://doc.telink-semi.cn/tools/bdt/Linux/BDT_Linux.zip)
   to `~/Downloads` and extract it:

   ```bash
   cd ~/Downloads
   unzip BDT_Linux.zip -d ~/tools/telink-bdt/
   ```

   The extracted `BDT_Linux` directory contains both the `TGui-BDT` and
   `Telink-BDT` packages.

4. Extract TGui-BDT:

   ```bash
   cd ~/tools/telink-bdt/BDT_Linux/
   tar -xzf TGui-BDT-Linux-V1.0.2.tar.gz -C ~/tools/telink-bdt/
   ```

5. Launch TGui-BDT:

   ```bash
   cd ~/tools/telink-bdt/TGui-BDT-Linux-V1.0.2/
   sudo ./TGui
   ```

#### Step 3: Flash the firmware

1. Select the chip and firmware. Choose `TL323X` as the chip, then select the
   firmware binary to flash, such as `build_blinky/zephyr/zephyr.bin`.

   ![Select the TL323X chip and firmware binary](figures/tgui_chip_bin.png)

2. Erase the flash. Set the erase size to **2040 KB**, then erase. The final
   8 KB of a 2 MB flash device is reserved for SoC data.

   ![Unlock and erase the flash](figures/tgui_unlock_erase.png)

3. Verify SWS, unlock, and download. Click **SWS** first to verify that
   communication with the target is working. Click **Unlock** to remove flash
   protection, then click **Download** to program the selected firmware.

   ![Verify SWS, unlock, and download](figures/tgui_sws_unlock_download.png)

4. Reset the target. After the download completes, click **Reset** to restart
   the board and run the new firmware.

   ![Reset the target in TGui](figures/tgui_reset.png)

### Verify the running results

After flashing, verify that the Blinky sample runs correctly.

**Operation steps**

1. After flashing, the development board resets automatically. If it does not,
   press the **Reset** button on the development board.
2. The Blinky sample toggles the onboard LED. Confirm that the LED blinks at a
   steady interval.

   ![Blinky](figures/tgui_blinky.png)

**Expected results**

When the program runs normally, the onboard LED blinks at a steady interval
(default frequency is approximately 1 Hz).

---

## Documentation

| Document                                                                                                                            | Description                                                                                       |
| ----------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| [Telink Matter Developer Guide](https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/) | Telink Matter SDK software architecture, repository structure, and functional module descriptions |

## Community and resources

| Resource                                                                                   | Description                                                     |
| ------------------------------------------------------------------------------------------ | --------------------------------------------------------------- |
| [Telink Official Forum](https://forum.telink-semi.cn/)                                     | Technical exchange and support                                  |
| [Telink Official Website](https://www.telink-semi.com/)                                    | Product center and documentation center                         |
| [Zephyr Project](https://github.com/zephyrproject-rtos/zephyr/blob/v4.1-branch/README.rst) | Zephyr official community                                       |
| [Zephyr Project Documentation](https://docs.zephyrproject.org/latest/)                     | Zephyr technical documentation                                  |
| [Zephyr Samples](https://docs.zephyrproject.org/latest/samples/index.html#samples)         | Zephyr sample programs                                          |
| [API Reference](https://docs.zephyrproject.org/4.1.0/doxygen/html/index.html)              | API reference                                                   |
| [GitHub](https://github.com/telink-semi/tl_zephyr)                                          | SDK source repository                                           |
| [Contribution Guide](https://docs.zephyrproject.org/latest/contribute/index.html)          | Issue submission, code contribution, and development guidelines |

---

## Appendix 1: Frequently Asked Questions

### Chip status shows `Not Activate`

**Problem**

BDT displays the following message when connecting to the chip:

```text
Not Activate
```

**Solution**

Before clicking **Download**, click **Activate** to activate the chip first.
After "**Activate MCU ok**" appears, proceed with the **Download** operation to
flash.

### Blinky sample LED does not blink after running

**Problem**

After flashing and resetting the development board, the onboard LED does not
blink as expected.

**Solution**

Check the following:

- Confirm that the firmware was flashed successfully.
- Confirm that the development board is powered correctly.
- Check that the development board jumper configuration is correct.
- Confirm that the current project configuration matches the development board.
