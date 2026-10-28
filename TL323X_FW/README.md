# TL323X_FW Prebuilt Firmware Binaries

## Overview

This directory contains prebuilt firmware binaries for the Telink TL323X
platform. They were added in PR #716 (`tl323x: Add MCUBOOT_START_OFFSET to
support SecureBoot`) to verify the `MCUBOOT_START_OFFSET` (0x5000) boot
offset feature and Secure Boot jump-to-application behavior.

## Contents

| File | Description |
|---|---|
| `SBOOT/Secure_Bootloader.bin` | Secure Bootloader test build (see warning below) |
| `SBOOT/Secure_Bootloader-no_logging.bin` | Secure Bootloader test build without logging |
| `SBOOT/DEBUG/Secure_Bootloader-Disable_Verification.bin` | Debug build with signature verification **disabled** |
| `SBOOT/DEBUG/Secure_Bootloader-Disable_Verification-logPB2.bin` | Debug build, verification disabled, log on PB2 |
| `SBOOT/DEBUG/Secure_Bootloader-logPB2_togglePB0.bin` | Debug build, log on PB2, PB0 toggle |
| `ZB/sampleSwitch_tl323x-bak.bin` | Zigbee 3.0 sample switch application, test build |

## WARNING - FOR DEVELOPMENT AND TESTING ONLY

**The binaries in this directory are NOT release firmware. Do NOT use them
in production.**

Key points:

- All binaries are internal test artifacts compiled from a debug
  environment. They do not match any official Secure Boot SDK release.
- `SBOOT/Secure_Bootloader.bin` is byte-identical to
  `SBOOT/DEBUG/Secure_Bootloader-Disable_Verification.bin`, i.e. it has
  signature verification **disabled**. Flashing it provides no Secure
  Boot protection whatsoever.
- `ZB/sampleSwitch_tl323x-bak.bin` is a manually backed-up (`-bak`)
  test image used for boot-offset validation.

## Production Firmware

For production, use the official Secure Boot SDK release branch
(`tl323x_SW_boot_release_v1.0` of the `secure_boot` repository) and build
the bootloader from source. Verify the sha256 checksum of any production
bootloader image before flashing.
