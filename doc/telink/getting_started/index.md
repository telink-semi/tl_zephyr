# Telink Zephyr SDK Getting Started Guide

[![Version](https://img.shields.io/badge/Version-tl_v1.0.1--rc1--v4.1.0-blue?style=flat-square)](https://github.com/telink-semi/tl_zephyr/releases/tag/tl_v1.0.1-rc1-v4.1.0)
[![Zephyr](https://img.shields.io/badge/Zephyr-v4.1.0-green?style=flat-square)](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.1.0)
[![License](https://img.shields.io/badge/License-Apache%202.0-red?style=flat-square)](../../../LICENSE)

---

Follow this guide to:

<!-- - Set up a command-line Telink Zephyr development environment on Ubuntu
- Get the Telink Zephyr source code (forked from the upstream Zephyr project)
- Install the Zephyr SDK toolchain (`riscv64-zephyr-elf`)
- Build and flash a sample application on Telink SoCs (TLSR9518ADK80D, TLSR9528A, TL321X, TL721X, TL323X, etc.) -->

- Set up a command-line Telink Zephyr development environment on Ubuntu
- Get the Telink Zephyr source code (forked from the upstream Zephyr project)
- Install the Zephyr SDK toolchain (`riscv64-zephyr-elf`)
- Build and flash a sample application on Telink SoC (TL323X)

> 💡 This guide is the Telink-flavoured counterpart of the upstream
> [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
> The main differences are: the Telink fork repository/branch, the Telink HAL
> binary blob fetch step, and the RISC-V toolchain selection. For the full
> walkthrough (with screenshots and detailed tips) see the
> **Telink Matter Developer Guide** → Chapter *Install Zephyr Project Environment*.

---

## Select and Update OS

This guide covers **Ubuntu 24.04 LTS**. If you are using a
different Linux distribution see
[Install Linux Host Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/installation_linux.html#installation-linux).

```bash
sudo apt update
sudo apt upgrade
```

---

## Hardware and Software Requirements

Hardware and software requirements to prepare before running the SDK.
|Hardware|Description|
|-|-|
|PC|Linux distro as ubuntu 24.04 LTS|
|Development board|Select a suitable development board according to the [Telink Zephyr SDK Release Note](doc/telink/releases/release-notes-tl_v1.0.1.md), e.g. TL323X Evaluation Kit in this guide.|
|Programmer|Programmer V5|
|USB cable|Connects the PC and the Programmer|
|Dupont wires|Connect the development board and the Programmer|

|Software|Description|
|-|-|
|Toolchain| RISC-V 64-bit Zephyr SDK Toolchain |
|Burning Tool | [Telink BDT for Linux](https://doc.telink-semi.cn/tools/bdt/Linux/BDT_Linux.zip) (Burning and Debugging tool) |
|SDK|[Telink Zephyr SDK](https://github.com/telink-semi/tl_zephyr)  |


## Install Dependencies

Next, install the host tools Zephyr needs to configure and build applications.

The current minimum required versions for the main dependencies are:

| Tool                                               | Min. Version |
| -------------------------------------------------- | ------------ |
| [CMake](https://cmake.org/)                        | 3.20.0       |
| [Python](https://www.python.org/)                  | 3.6          |
| [Devicetree compiler](https://www.devicetree.org/) | 1.4.6        |

1. Install the Kitware APT repository (provides a recent CMake), then install
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
   > Due to the unavailability of `gcc-multilib` and `g++-multilib` on AArch64
   > (ARM64) systems, you may need to omit them from the list of packages to
   > install.

2. Verify the versions of the main dependencies installed on your system:

   ```bash
   cmake --version
   python3 --version
   dtc --version
   ```

   Check those against the versions in the table above. If the APT mirror ships
   outdated versions, switch to a stable, up-to-date mirror or update the
   dependencies manually.

---

## Install west

[west](https://docs.zephyrproject.org/latest/develop/west/index.html#west) is
Zephyr's multi-repository meta-tool.

1. Create a new virtual environment:

   ```bash
   python3 -m venv ~/zephyrproject/.venv
   ```

2. Activate the virtual environment:

   ```bash
   source ~/zephyrproject/.venv/bin/activate
   ```
   Once activated your shell will be prefixed with `(.venv)`. The virtual environment can be deactivated at any time by running `deactivate`.

   > **Note**
   > Remember to activate the virtual environment every time you start a new terminal session before working with Zephyr. If you don’t, commands such as west will not be found, or may run against a different Python environment, leading to confusing errors.

3. Install west:

   ```bash
   pip3 install west
   ```

---

## Get the Telink Zephyr Source Code

The Telink Zephyr SDK is a fork of the upstream
[zephyrproject-rtos/zephyr](https://github.com/zephyrproject-rtos/zephyr)
repository, hosted at [telink-semi/tl_zephyr](https://github.com/telink-semi/tl_zephyr).
The Telink-specific changes live on the `release-v1.0-v4.1-branch` branch.

1. Initialize the west workspace with the upstream manifest (this creates the
   `~/zephyrproject` workspace and clones the upstream Zephyr repository as the
   manifest repository):

   ```bash
   west init ~/zephyrproject
   cd ~/zephyrproject
   west update
   ```

   > **Tip — China mainland network**
   > Running `west init` and `west update` to fetch the Zephyr source can take
   > extra time in China mainland, and some projects may fail to update from
   > foreign servers. Use a mirror or proxy, or download the source bundle
   > separately.

2. Export the Zephyr CMake package so `find_package(Zephyr)` can locate this
   checkout:

   ```bash
   west zephyr-export
   ```

3. Install Zephyr's Python dependencies:

   ```bash
   pip3 install -r ~/zephyrproject/zephyr/scripts/requirements.txt
   ```

4. Add the Telink remote and check out the Telink `release-v1.0-v4.1-branch` branch:

   ```bash
   cd ~/zephyrproject/zephyr
   git remote add telink https://github.com/telink-semi/tl_zephyr
   git fetch telink
   git checkout release-v1.0-v4.1-branch
   cd ..
   west update
   ```

5. Fetch the Telink HAL (BLE stack). The fetch method depends on your chip
   family — see [Fetch the Telink HAL](#fetch-the-telink-hal) below.

---

## Fetch the Telink HAL

The Telink BLE stack (HAL) is shipped as a binary blob. How you fetch it
depends on the chip family:

- **hal_v1** chips (TLSR9518ADK80D, TLSR9528A, TL321X, TLSR9118) — the HAL is
  fetched with `west blobs`. Re-run it whenever you switch branches or update
  the workspace:

  ```bash
  west blobs fetch hal_telink
  ```

- **hal_v2** chips (TL322X, TL323X, TL721X) — the BLE stack is fetched via the
  `fetch_sdk.sh` script instead of `west blobs`:

  ```bash
  cd ~/zephyrproject/modules/hal/telink/hal_v2
  chmod +x fetch_sdk.sh
  ./fetch_sdk.sh
  ```

  > **Note**
  > `fetch_sdk.sh` uses a default repository (`telink-semi/tl_ble_sdk_zephyr`)
  > and the latest commit. To pin a specific repository/commit, pass them as
  > arguments:
  > ```bash
  > ./fetch_sdk.sh https://github.com/telink-semi/tl_ble_sdk_zephyr.git f50d422d780efb73af93b650ef7b8c6bf5a0b99b
  > ```

<!-- | HAL version | Applicable chips                            | Fetch command                 |
| ----------- | ------------------------------------------- | ----------------------------- |
| **hal_v1**  | TLSR9518ADK80D, TLSR9528A, TL321X, TLSR9118 | `west blobs fetch hal_telink` |
| **hal_v2**  | TL322X, TL323X, TL721X                      | `./hal_v2/fetch_sdk.sh`       | -->

For TL323X platform, use the following command:
| HAL version | Applicable chips                            | Fetch command                 |
| ----------- | ------------------------------------------- | ----------------------------- |
| **hal_v2**  | TL323X                      | `./hal_v2/fetch_sdk.sh`       |

---

## Switching Branches or Commits

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
[*Fetch the Telink HAL*](#fetch-the-telink-hal) above, choosing the version
that matches your chip.

---

## Install the Zephyr SDK Toolchain

The [Zephyr SDK](https://docs.zephyrproject.org/latest/toolchains/zephyr_sdk.html#toolchain-zephyr-sdk)
contains the toolchains for each of Zephyr's supported architectures. Telink
SoCs are RISC-V based, so only the `riscv64-zephyr-elf` toolchain is required.

1. Download the Zephyr SDK **v0.17.0** minimal archive:

   ```bash
   wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64_minimal.tar.xz
   ```

2. Verify the download:

   ```bash
   wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/sha256.sum | shasum --check --ignore-missing
   ```

3. Extract the archive to one of the recommended paths (for example
   `~/zephyr-sdk-0.17.0`):

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

4. Install only the RISC-V toolchain:

   ```bash
   cd zephyr-sdk-0.17.0
   ./setup.sh -t riscv64-zephyr-elf -h -c
   ```

> **Note — Full SDK**
> If you need the full Zephyr SDK (with host tools such as QEMU and OpenOCD),
> replace the minimal archive with the full one:
> ```bash
> wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64.tar.xz
> wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/sha256.sum | shasum --check --ignore-missing
> tar xvf zephyr-sdk-0.17.0_linux-x86_64.tar.xz ~/zephyr-sdk-0.17.0
> cd zephyr-sdk-0.17.0
> ./setup.sh -t riscv64-zephyr-elf -h -c
> ```

---

## Add the Zephyr Environment to Your Shell

Append the Zephyr environment script to `~/.bashrc` so that `west build` can
find the workspace in every new terminal:

> **Warning**
> If you skip this step you may hit `west build` errors later.

```bash
echo "source ~/zephyrproject/zephyr/zephyr-env.sh" >> ~/.bashrc
source ~/.bashrc
```

---

## Build the Blinky Sample

Before building a Telink application, verify that the Zephyr environment is set
up correctly by building the *Blinky* sample.

From the Zephyr root directory:

```bash
cd ~/zephyrproject/zephyr
```

Build *Blinky* for your Telink board.

<!-- Replace `<board-name>` with one of the supported Telink board targets. e.g. `tl3238x`: -->

```bash
west build -p auto -b tl3238x samples/basic/blinky -d build_blinky
```

<!-- Supported Telink board targets include: -->

<!-- | Board target      | SoC            |
| ----------------- | -------------- |
| `tlsr9518adk80d`  | TLSR9518ADK80D |
| `tlsr9528a`       | TLSR9528A      |
| `tl3218x`         | TL3218X        |
| `tl3238x`         | TL3238X        |
| `tl7218x`         | TL7218X        | -->


After the build completes, you will find `zephyr.bin` in the
`build_blinky/zephyr/` folder.

---

## Evaluation Boards Overview

Telink provides a range of evaluation boards (EVBs) for the supported SoC
families. The table below lists the EVBs covered by this SDK. For full
schematics, jumper configurations, button/LED layouts and power-supply options,
see the **Telink Matter Developer Guide** → Chapter *Required Equipment*.

<!-- | EVB              | SoC        | Key interfaces                                 |
| ---------------- | ---------- | ---------------------------------------------- |
| TLSR9518ADK80D   | TLSR9518   | Mini-USB (power/flash), UART header, buttons   |
| TLSR9528A        | TLSR9528   | Mini-USB (power/flash), UART header, buttons   |
| TL3218X          | TL3218X    | Type-C (power), UART header, buttons            |
| TL3228X          | TL3228X    | Type-C (power), UART header, buttons            |
| TL3238X          | TL3238X    | Type-C (power), UART header, buttons            |
| TL7218X          | TL7218X    | Type-C (power), UART header, buttons            | -->

| EVB              | SoC        | Key interfaces                                 |
| ---------------- | ---------- | ---------------------------------------------- |
| TL3238X          | TL3238X    | Type-C (power), UART header, buttons            |

Each EVB exposes a set of mechanical buttons (factory reset, BLE advertising,
light control, network commissioning) and status LEDs.

UART serial output is
available at **115200 baud, 8 data bits, no parity, 1 stop bit** — see the
developer handbook for the exact TX/RX/GND pinout of your board.

---

## Install the Flashing Tool (BDT)

Telink Burning Debug Tool (BDT) is the official flasher for Telink SoCs.
It is available for both Windows and Linux. This guide use Linux BDT only.

<!-- ### Windows -->

<!-- Download BDT from the Telink wiki and extract it to a local folder:

- BDT (Windows): <https://doc.telink-semi.cn/tools/bdt/Windows/BDT.zip>

The archive bundles **two GUI applications** — choose the one that matches
your programmer hardware:

- **`Telink BDT.exe`** (release V5.9.x) — the classic GUI. Used with an
  external **Burning EVK V1.0–V3.0** (SWS interface). Supports all current
  chips: B91, B92, TL321X, TL322X (V5.8.4+), TL323X (V5.9.0+), TL721X,
  TL751X, and older 8xxx/B8x chips.
- **`TGui/TGui.exe`** — the new web-based GUI. Used with the **on-board
  programmer (Burning EVK V4.0)** or **Programmer V5** (USB/serial interface,
  no external EVK jig needed). Supports TL321X, TL322X, TL323X, TL721X,
  TC321X. On first use, click the **Install drv** button inside TGui to
  install the USB driver, then click **Refresh** to enumerate the board. -->

  <!-- - **`Telink BDT.exe`** (release V5.9.x) — the classic GUI. Used with an
  external **Burning EVK V1.0–V3.0** (SWS interface). Supports chips:TL323X (V5.9.0+), TL721X.

  - BDT (Windows): <https://doc.telink-semi.cn/tools/bdt/Windows/BDT.zip>

> **Note — msvcr120 dependency**
> If `Telink BDT.exe` fails to open, the Microsoft Visual C++ 2013 runtime
> is missing. Copy the `msvcr120.dll` from the bundled `msvcr120/` folder
> to `C:\Windows\System32` (32-bit Windows) or `C:\Windows\SysWOW64`
> (64-bit Windows). -->

<!-- The detailed flashing procedures are described in
[Option A: Windows (BDT GUI)](#option-a-windows-bdt-gui) (Telink BDT, EVK
V1–V3) and [Option A2: Windows (TGui)](#option-a2-windows-tgui-tl322x--tl323x)
(TGui, on-board programmer) below. -->

<!-- ### Linux (Ubuntu 24.04)

Download and extract the Linux BDT package:

```bash
wget https://doc.telink-semi.cn/tools/bdt/Linux/BDT_Linux.zip
unzip BDT_Linux.zip
```

The package contains two sub-packages — choose the one that matches your
**Burning EVK** version:

- **Telink-BDT-Linux-X64-2.1.0** — for **Burning EVK V1.0–V3.0** (the black
  box). Supports B91, B92, TL321X, TL721X, TL751X. Provides a GUI (`bdt_gui`)
  and a CLI (`bdt`). Requires EVK firmware **V4.6** or later.
- **TGui-BDT-Linux-V1.0.2** — for **Burning EVK V4.0** (on-board programming
  module). Supports TL321X, TL322X, TL323X, TL721X, TC321X. Provides a
  web-based GUI (`TGui`) and a CLI (`sctool`).

The CLI steps in [Option B](#option-b-linux-linuxbdt-cli) below use the
`bdt` tool from the first package. If your board requires TGui-BDT (e.g.
TL322X, TL323X), refer to the documentation bundled with the TGui package. -->

### Linux (Ubuntu 24.04)

This guide uses **TGui-BDT** to flash the **TL323X**.

1. **Install the required dependencies:**

   ```bash
   sudo apt update

   sudo apt install -y \
       libgtk-3-dev \
       libusb-1.0-0-dev
   ```

2. **Create a directory for the BDT tools:**

   ```bash
   mkdir -p ~/tools/telink-bdt
   ```

3. **Download and extract the Linux BDT package.**

   Download [BDT_Linux.zip](https://doc.telink-semi.cn/tools/bdt/Linux/BDT_Linux.zip)
   to `~/Downloads`, then run:

   ```bash
   cd ~/Downloads
   unzip BDT_Linux.zip -d ~/tools/telink-bdt/
   ```

   The extracted `BDT_Linux` directory contains both the `TGui-BDT` and
   `Telink-BDT` packages.

4. **Extract TGui-BDT:**

   ```bash
   cd ~/tools/telink-bdt/BDT_Linux/
   tar -xzf TGui-BDT-Linux-V1.0.2.tar.gz -C ~/tools/telink-bdt/
   ```

5. **Launch TGui:**

   ```bash
   cd ~/tools/telink-bdt/TGui-BDT-Linux-V1.0.2/
   sudo ./TGui
   ```

---

## Flash the Firmware

Before flashing, connect the hardware:

1. Connect the **Burning EVK** (flashing jig) to the target EVB using the
   supplied cable.
2. Connect the Burning EVK to your host PC with a Mini-USB cable.
3. Keep the **default jumper configuration** on the EVB — do not change any
   jumpers unless instructed otherwise.

A **Telink Burning EVK** and two Mini-USB cables are required per board. When
flashing multiple boards simultaneously you will need additional Burning EVKs
and a USB hub with enough ports.

<!-- > **Note — Chip-to-BDT name mapping**
> Each SoC family uses a slightly different name in BDT. On Windows both
> `Telink BDT.exe` and `TGui` recognise the GUI chip names below; on Linux
> the `bdt` CLI (v2.1.0) uses slightly different casing (third column).
>
> | Board target      | BDT GUI chip selection | Linux `bdt` chip name | Notes                              |
> | ----------------- | ---------------------- | --------------------- | ---------------------------------- |
> | `tlsr9518adk80d`  | `B91`                  | `B91`                 |                                    |
> | `tlsr9528a`       | `B92_3V3`              | `B92`                 |                                    |
> | `tl3218x`         | `TL321X`               | `tl321x`              |                                    |
> | `tl3228x`         | `TL322X`               |                    | Windows: BDT V5.8.4+ (EVK mode)    |
> | `tl3238x`         | `TL323X`               |                    | Windows: BDT V5.9.0+ (EVK mode)    |
> | `tl7218x`         | `TL721X`               | `tl721x`              |                                    |
>
> ¹ TL322X and TL323X are **not** supported by the Linux `bdt` command-line
> tool (v2.1.0). On Windows, `Telink BDT.exe` supports them in EVK mode
> (Burning EVK V1–V3) from V5.8.4 / V5.9.0 onwards, and `TGui` supports them
> via the on-board programmer V4.0. On Linux, use the `sctool` from the
> TGui-BDT package (see [Option C](#option-c-linux-tgui-bdt-cli-tl322x--tl323x-only)).
>
> ² B92 has a 1.8 V variant `B92_1V8` as well; the TLSR9528A EVB uses
> `B92_3V3` (3.3 V). -->

> **Note — Chip-to-BDT name mapping**
> Each SoC family uses a slightly different name in BDT. On Linux
> `TGui` recognise the GUI chip names below.
>
> | Board target      | BDT GUI chip selection |
> | ----------------- | ---------------------- |
> | `tl3238x`         | `TL323X`               |
<!-- >
> `Telink BDT.exe` supports them in EVK mode
> (Burning EVK V1–V3) from V5.8.4 / V5.9.0 onwards.

Choose the flashing procedure that matches your host OS and programmer
hardware: -->

<!-- - **Windows** (all supported chips, external Burning EVK V1.0–V3.0) →
  [Option A: Windows (BDT GUI)](#option-a-windows-bdt-gui)
- **Windows, TL322X/TL323X** with the on-board programmer V4.0 (alternative
  to Option A) → [Option A2: Windows (TGui)](#option-a2-windows-tgui-tl322x--tl323x)
- **Linux, B91/B92/TL321X/TL721X** (Burning EVK V1.0–V3.0) →
  [Option B: Linux (LinuxBDT CLI)](#option-b-linux-linuxbdt-cli)
- **Linux, TL322X/TL323X** (Burning EVK V4.0 on-board) →
  [Option C: Linux (TGui-BDT CLI)](#option-c-linux-tgui-bdt-cli-tl322x--tl323x-only) -->

  <!-- - **Windows** (all supported chips, external Burning EVK V1.0–V3.0) →
  [Option A: Windows (BDT GUI)](#option-a-windows-bdt-gui)

### Option A: Windows (BDT GUI) -->

  <!-- - **Windows** (all supported chips, external Burning EVK V1.0–V3.0) →
  [Windows (BDT GUI)](#windows-bdt-gui) -->

<!-- ### Windows (BDT GUI) -->

<!-- > Covers all supported chips (B91, B92, TL321X, TL322X, TL323X, TL721X) with
> an external **Burning EVK V1.0–V3.0**. TL322X requires BDT V5.8.4 or later;
> TL323X requires V5.9.0 or later. If your board has an on-board programmer
> V4.0 and you prefer the TGui workflow, see
> [Option A2](#option-a2-windows-tgui-tl322x--tl323x). -->

<!-- This flash sample is implemented for the TL323X platform.

1. **Connect to Hardware.** Before using the BDT tool, connect the PC, programmer, and target board as follows:
   - **PC ↔ Programmer**: Connect them using a USB cable. If the green indicator LED on the programmer stays solid, the programmer has been successfully recognized by the PC.
   - **Programmer ↔ Target board**: Connect them using Dupont wires:
     - Power lines: 3V3 ↔ 3V3; GND ↔ GND
     - Data line (single-wire SWM bus): Connect the programmer's SWM pin to the target board's SWS (Swire) pin.

   ![Connect_Hardware](figures/Connect_Hardware.png)
2. **Launch BDT.** Double-click `Telink BDT.exe`.

3. **Select the programmer.** From the **Programmer** drop-down, choose
   **Programmer V1-V3** (the external Burning EVK). The other entries
   (`On-board Programmer`, `9118 Programmer`, `Programmer V5`) are for
   different hardware and will not work with the V1–V3 EVK.

   ![Select programmer](figures/Select_Programmer.png)

   If the EVK is connected successfully, the EVK device information appears in the window title bar.

   ![Launch BDT](figures/Launch_BDT.png) -->

<!-- 4. **Select the chip.** From the chip-selection drop-down, choose the entry
   that matches your board (see the chip-mapping table above), e.g.
   `B92_3V3` for TLSR9528A, `TL321X` for TL3218X, `TL721X` for TL7218X,
   `TL322X` for TL3228X, or `TL323X` for TL3238X.
   > B92 has two variants — `B92_3V3` (3.3 V, used by the TLSR9528A EVB) and
   > `B92_1V8` (1.8 V). Pick the one that matches your board voltage. -->

   <!-- 4. **Select the chip.** From the chip-selection drop-down, choose the entry
   that matches your board `TL323X` for TL3238X.
   ![Select chip](figures/Select_Chip.png)

5. **Verify the connection (SWS).** Click the **SWS** button on the toolbar. If
   the connection is good, the log window displays the connected EVK and chip
   information. If you see a `Swire err!` message, see the troubleshooting
   notes at the end of this section.

   ![Verify connection](figures/Verify_Connection.png)

6. **Set the flash erase size.** Click the **Setting** button on the toolbar.
   The default erase size is 512 KB — change it to **2040** (2040 KB). For
   boards with 2 MB external flash the last 8 KB is reserved for SoC data, so
   the maximum erasable area is 2040 KB.

   ![Set flash erase size](figures/Set_Flash_Erase_Size.png) -->

<!-- 7. **Unlock the flash (if prompted).** Since BDT V5.7.8 the tool warns about
   flash protection before erase/download for B91/B92/TL321X/TL721X. Since
   V5.7.9 a **manual/auto unlock** toggle button is available on the toolbar.
   If you see a *flash is locked* warning, click the **Unlock** button (or set
   auto-unlock mode) before proceeding. The **Flash info** button shows the
   current MID, UID, status and lock address. -->

   <!-- 7. **Unlock the flash (if prompted).** Since BDT V5.7.8 the tool warns about
   flash protection before erase/download for TL323X. Since
   V5.7.9 a **manual/auto unlock** toggle button is available on the toolbar.
   If you see a *flash is locked* warning, click the **Unlock** button (or set
   auto-unlock mode) before proceeding. The **Flash info** button shows the
   current MID, UID, status and lock address.
   ![Unlock flash](figures/Unlock.png)

8. **Erase the flash.** Click the **Erase** button on the toolbar and wait for
   the erase to complete.

   ![Erase flash](figures/Erase_Flash.png)

9. **Open the firmware file.** Click **File → Open** and select the
   `zephyr.bin` file from your build directory (e.g. `build_blinky/zephyr/zephyr.bin`).
   The selected file name appears in the status bar at the bottom of the
   window.

10. **Flash the firmware.** Click the **Download** button on the toolbar and
    wait for the flash programming to complete.

11. **Reset the board.** After flashing completes, power-cycle the board (or
    press its reset button) to start the new firmware.

    ![Download_Reset](figures/Download_Reset.png)
For more BDT commands and options, refer to the documentation in the
`doc/` folder of the BDT package.

#### Windows BDT troubleshooting

- **`Telink BDT.exe` does not open** — the MSVC runtime is missing. Copy
  `msvcr120.dll` from the bundled `msvcr120/` folder to
  `C:\Windows\System32` (32-bit) or `C:\Windows\SysWOW64` (64-bit), as
  described in `readme.txt`.

- **EVK not detected by BDT** (EVK appears in Windows Device Manager but not
  in the BDT title bar): this is a known issue on some AMD-platform PCs. Try
  an Intel-based PC.

- **`Swire err!` after clicking SWS**: three common causes:
  1. *Hardware connection* — double-check all cables and jumper settings.
  2. *EVK firmware too old* — upgrade the EVK firmware.

      ![update warning](figures/Update_Warning.png)
      BDT V5.9.x bundles
     EVK firmware **V5.1** (file in `config\fw\`). Use **Help → Upgrade**,
     click **Read FW version** to check the current version, **Load…** the
     newest firmware file, click **Upgrade**, then re-plug the EVK USB cable.
     BDT V5.8.1+ also self-checks the EVK firmware version on power-up and
     can auto-update it (`Firmware_Auto_Update`).

     ![firmware upgrade](figures/Update_BDT_firmware.png)
   3. *Not Activate* - Send **Activate** Command to the chip.

      ![pre activate](figures/Pre_Activate.png) -->

<!-- ### Option A2: Windows (TGui, TL322X / TL323X)

An alternative to [Option A](#option-a-windows-bdt-gui) for **TL322X** and
**TL323X** boards that have an on-board programmer (Burning EVK V4.0) or an
external **Programmer V5**. TGui talks to the chip over USB/serial through
the on-board programmer, so no external Burning EVK jig is needed. Use the
**TGui** GUI (`TGui/TGui.exe`) bundled in the same BDT archive.

1. **Launch TGui.** Double-click `TGui/TGui.exe`.
2. **Install the driver (first use only).** Click the **Install drv** button.
   When installation completes, click **Refresh** to enumerate the connected
   board.
3. **Select the chip.** Choose `TL322X` or `TL323X` from the chip list.
4. **Connect.** TGui communicates over USB/serial — confirm the board appears
   in the device list.
5. **Erase & flash.** Use the TGui **Erase** and **Download** buttons (the
   workflow mirrors the classic BDT GUI: erase first, then open `zephyr.bin`
   via **File → Open**, then **Download**). TGui auto-handles flash unlock
   for TL322X/TL323X, so no manual unlock step is needed.
6. **Reset.** Power-cycle the board to start the new firmware.

Supported chips in TGui V1.0.x: TL721X, TL321X, TL322X, TL323X, TC321X.
For details, see the release note in `TGui/release_note.txt`.

### Option B: Linux (LinuxBDT CLI)

The `linux_release/` folder contains both a GUI (`bdt_gui`, whose workflow is
identical to the Windows BDT GUI described above) and a command-line tool
(`bdt`). The CLI steps are as follows.

1. **Connect the Burning EVK** to a USB port on your PC.

2. **Verify the connection (SWS).** From the `linux_release/` directory, check
   that the Burning EVK can see the target board. Replace `<chip>` with the
   BDT chip name from the chip-mapping table above (e.g. `B92`):

   ```bash
   ./bdt <chip> sws
   ```

   If the connection is good, the tool prints the connected EVK and board
   info.

3. **Activate the MCU.** When the target board already has firmware running, a
   direct erase/flash may fail with a `Swire err!`. Activate the MCU first:

   ```bash
   ./bdt <chip> ac
   ```

4. **Unlock the flash.** The flash may be locked while a program is running.
   For `B92`, `tl321x` and `tl721x`, unlock the flash before erasing:

   ```bash
   ./bdt <chip> ulf
   ```

   > **Note**
   > `B91` does not support the `ulf` command — skip this step for the
   > TLSR9518 board. Alternatively, you can pass `-f` (auto unlock) to the
   `wf` command in the next step instead of running `ulf` separately.

5. **Erase the flash.** Erase from address `0` to avoid leftover data from a
   previous (larger) image:

   ```bash
   ./bdt <chip> wf 0 -s 2040k -e
   ```

   - `wf` — write flash (used here for the erase sub-action)
   - `0` — start address
   - `-s 2040k` — sector size to erase (use `2040k` for 2 MB flash; the last
     8 KB of a 2 MB part is reserved for SoC info)
   - `-e` — erase

   Wait until the erase completes (a few tens of seconds).

6. **Write the firmware.** Flash the `zephyr.bin` you built earlier:

   ```bash
   ./bdt <chip> wf 0 -i build_blinky/zephyr/zephyr.bin
   ```

7. **Reset the MCU:**

   ```bash
   ./bdt <chip> rst
   ```

The board reboots and starts running the newly flashed firmware.

For more `bdt` commands and options, refer to the documentation in the
`doc/` folder of the LinuxBDT package.

### Option C: Linux (TGui-BDT CLI, TL322X / TL323X only)

For **TL322X** and **TL323X** boards, which integrate a Burning EVK V4.0
on-board programming module, use the `sctool` CLI bundled in the
**TGui-BDT-Linux** package. Unlike `bdt`, `sctool` talks to the chip over a
**serial UART** (no external Burning EVK jig is needed).

> **Prerequisite — dialout group**
> `sctool` accesses the board through a serial port, so your user must be a
> member of the `dialout` group. Add yourself once and reboot (or re-login):
> ```bash
> sudo usermod -a -G dialout $USER
> ```

The steps below assume you are in the `Apps/app2/CLI/` directory of the
extracted TGui-BDT package. Replace `/dev/ttyUSB0` with the actual serial
device of your board (find it with `./sctool list_ports`).

1. **Connect the board** to your PC with a USB cable. The on-board Burning
   EVK V4.0 module enumerates as a serial port (e.g. `/dev/ttyUSB0`).

2. **List serial ports and confirm the board is detected:**

   ```bash
   ./sctool list_ports
   ```

3. **Get the chip ID** to verify the connection (this also uploads the
   download agent automatically):

   ```bash
   ./sctool -p /dev/ttyUSB0 chip_id
   ```

4. **Erase the flash.** Erase the whole flash, or a region starting at `0x0`:

   ```bash
   # Erase the whole flash (recommended for a clean image)
   ./sctool -p /dev/ttyUSB0 flash_chip_erase

   # …or erase a specific region: address size (in bytes)
   ./sctool -p /dev/ttyUSB0 flash_erase 0x0 0x200000
   ```

5. **Write the firmware.** Flash the `zephyr.bin` you built earlier. `sctool`
   erases the affected sectors automatically unless you pass `--no-erase`, so
   the explicit erase in the previous step is optional:

   ```bash
   ./sctool -p /dev/ttyUSB0 flash_write 0x0 build_blinky/zephyr/zephyr.bin
   ```

   Add `--verify` to read back and compare after writing:
   ```bash
   ./sctool -p /dev/ttyUSB0 flash_write 0x0 build_blinky/zephyr/zephyr.bin --verify
   ```

6. **Reset the board** to start the new firmware:

   ```bash
   ./sctool -p /dev/ttyUSB0 hw_reset
   ```

Common `sctool` options (placed before the sub-command):

- `-p <port>` — serial port (e.g. `/dev/ttyUSB0`)
- `-b <baudrate>` — baud rate for the download agent (default `2000000`)
- `-da <path>` — download-agent binary (defaults to `da/da.ram.bin` in the
  package, rarely needs overriding)
- `--before {hw_reset,no_reset}` — reset action before uploading the DA
- `--after {hw_reset,sw_reset,no_reset}` — reset action after the operation

Other useful sub-commands: `chip_info`, `flash_size`, `flash_read`, `efuse_read`.

For the full command reference, see `Telink_Command_Line_User_Guide.pdf` in
the `Apps/app1/cmd_tool/doc/` folder of the TGui-BDT package.

> For Matter applications that produce a `merged.bin` (MCUboot + application),
> flash `merged.bin` instead of `zephyr.bin`. See the per-board
> `*_README.md` build guides in the Matter repository for details. -->

### Linux (TGui-BDT GUI, TL323X)

This flashing example uses the **TL323X** and the TGui application launched in
the previous section.

1. **Connect the device.** Connect the programmer and target board to the
   Ubuntu host, then confirm that TGui detects the programmer.
   - **PC ↔ Programmer**: Connect them using a USB cable. If the green indicator LED on the programmer stays solid, the programmer has been successfully recognized by the PC.
   - **Programmer ↔ Target board**: Connect them using Dupont wires:
     - Power lines: 3V3C ↔ 3V3; GND ↔ GND
     - Data line (single-wire SWM bus): Connect the programmer's SWM pin to the target board's SWS (Swire) pin.

   ![Connect the device in TGui](figures/tgui_connect.png)

2. **Select the chip and firmware.** Select `TL323X` as the chip, then choose
   the firmware binary to flash, such as
   `build_blinky/zephyr/zephyr.bin`.

   ![Select the TL323X chip and firmware binary](figures/tgui_chip_bin.png)

3. **Erase the flash.** Set the erase size to **2040 KB**, then erase the
   flash. The final 8 KB of a 2 MB flash device is reserved for SoC data.

   ![Unlock and erase the flash](figures/tgui_unlock_erase.png)

4. **Verify SWS, unlock, and download.** Click **SWS** first to verify that
   communication with the target is working. Click **Unlock** to remove flash
   protection, then click **Download** to program the selected firmware.

   ![Verify SWS, unlock, and download](figures/tgui_sws_unlock_download.png)

5. **Reset the target.** After the download completes, click **Reset** to
   restart the board and run the new firmware.

   ![Reset the target in TGui](figures/tgui_reset.png)

---

## Verify the Result

After reset, confirm that the Blinky sample is running:

<!-- ### 1. Check the LED

The Blinky sample toggles an LED on the board. You should see the onboard LED
blink at a steady interval (~1 Hz by default). If the LED does not blink,
re-check the flash steps and the board's power/jumper configuration.

![Blinky](figures/Blinky.png)

### 2. Check the serial output

Connect a USB-to-UART module to the board's UART pins (see the developer
handbook for the pinout of your EVB) and open a serial terminal at **115200
8N1**:

```bash
# Replace /dev/ttyUSB0 with your UART module device
picocom -b 115200 /dev/ttyUSB0
```

When the Blinky sample starts successfully, you should see boot banner and
periodic LED state messages similar to:

```
*** Booting Zephyr OS build v4.0.0-6264-g540f4f100b37 ***
LED state: OFF
LED state: ON
LED state: OFF
LED state: ON
LED state: OFF
LED state: ON
...
```

The `LED state: OFF/ON` messages toggle each time the LED changes state.

For other samples that produce log output (e.g. *hello_world*), check for the
expected console message in the same way. -->


### Check the LED

The Blinky sample toggles an LED on the board. You should see the onboard LED
blink at a steady interval (~1 Hz by default). If the LED does not blink,
re-check the flash steps and the board's power/jumper configuration.

![Blinky](figures/tgui_blinky.png)

---

## Next Steps

Here are some next steps for exploring the Telink Zephyr SDK:

- Try other [Samples and Demos](https://docs.zephyrproject.org/latest/samples/index.html#samples)
  from the upstream Zephyr documentation.
- Build Telink Matter applications — see the
  [Telink Matter SDK README](https://github.com/telink-semi/connectedhomeip)
  and the per-board `*_README.md` build guides.
- Read the [Telink Zephyr SDK Release Note](../releases/release-notes-tl_v1.0.1.md) for
  the list of supported chips, features and known issues.
- Learn about [west](https://docs.zephyrproject.org/latest/develop/west/index.html#west)
  and [Application Development](https://docs.zephyrproject.org/latest/application/index.html#application)
  in the upstream Zephyr docs.
- Check the **Telink Matter Developer Guide** for flashing, debugging and guidance.

---

## Asking for Help

Before asking for help, search this document, the Telink Matter Developer
Guide, and the upstream Zephyr documentation. Your question may already have an
answer there.

- **Telink Matter Developer Guide**: https://doc.telink-semi.cn/doc/en/software/res/sdk/matter/telink_matter_developer_guide_en/
- **Upstream Zephyr docs**: https://docs.zephyrproject.org/
- **GitHub (Telink fork)**: https://github.com/telink-semi/tl_zephyr

When asking for help, include:

1. What you want to do
2. What you tried, including the commands you ran
3. What happened, including the full text output

Copy and paste text instead of sharing screenshots. For more than 5 lines of
terminal output, source code, or logs, create a snippet using three backticks.