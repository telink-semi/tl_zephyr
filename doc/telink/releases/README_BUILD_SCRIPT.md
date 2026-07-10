# Telink Build and Release Notes Generation Script

## Overview

The `build_and_update_notes.py` script can:
1. Build all samples for all Telink boards
2. Extract memory usage information from build logs
3. Generate a versioned release notes file from the `release-notes-template.md` template (replacing the `{{VERSION}}` placeholder with the target version and updating the Resource Usage section)
4. Collect all firmware files for all boards' samples and create a separate archive per board
5. Organize all build outputs under the `zephyr/build_for_release/` directory

## Files

All scripts and release notes files are located in the `zephyr/doc/telink/releases/` directory:

- `build_and_update_notes.py` - Main build and generation script
- `README_BUILD_SCRIPT.md` - This file
- `release-notes-template.md` - Release notes **template** file (always read as input by the script, never modified; uses `{{VERSION}}` as the version placeholder)
- `release-notes-<release-version>.md` - Generated release notes **output** file (written by the script)
- `zephyr/TELINK_VERSION` - Version definition file (auto-read by the script to infer the version)
- `zephyr/build_for_release/` - Root directory for all build outputs
  - `build_logs/` - Directory containing all build logs
  - `firmware/` - Directory containing all board firmware files and archives

## Version Auto-Detection

When the version is not explicitly specified via `--release-version`, the script automatically reads the version from `zephyr/TELINK_VERSION` and assembles it into the `tl_vX.Y.Z[.W][extra]` format.

Example `TELINK_VERSION` file:

```
VERSION_MAJOR = 1
VERSION_MINOR = 0
PATCHLEVEL = 1
VERSION_TWEAK = 0
EXTRAVERSION =
```

Assembly rules:
- Base format: `tl_v{MAJOR}.{MINOR}.{PATCHLEVEL}` (e.g. `tl_v1.0.1`)
- If `VERSION_TWEAK` is non-zero, append `.{TWEAK}` (e.g. `tl_v1.0.1.1`)
- If `EXTRAVERSION` is non-empty, append it directly (e.g. `tl_v1.0.1-rc1`)

### Filename Conflict Handling

If the generated output file `release-notes-<version>.md` already exists, the script automatically appends a `_2`, `_3`, ... suffix to avoid overwriting:

- `release-notes-tl_v1.0.1.md` already exists -> generate `release-notes-tl_v1.0.1_2.md`
- `release-notes-tl_v1.0.1_2.md` also exists -> generate `release-notes-tl_v1.0.1_3.md`
- And so on

## Usage

### 1. Full Run (Build + Generate + Firmware Packaging)

```bash
cd /home/ubuntu/zephyrproject/zephyr/doc/telink/releases
python3 build_and_update_notes.py
```

This will:
- Build all samples for all boards
- Save logs to `zephyr/build_for_release/build_logs`
- Extract memory usage information from logs
- Auto-detect the version from `TELINK_VERSION`, generate the release notes file from `release-notes-template.md`
- Collect all firmware files into `zephyr/build_for_release/firmware/`
- Create a separate firmware archive per board

### 2. Generate Release Notes Only (Skip Build)

```bash
python3 build_and_update_notes.py --skip-build
```

Use this when build logs already exist and you only need to generate release notes and collect firmware.

### 3. Skip Firmware Collection and Packaging

```bash
python3 build_and_update_notes.py --skip-firmware
```

Use this option when firmware collection and packaging are not needed.

### 4. Auto-Detect Version (Default Behavior)

```bash
# When --release-version is not specified, the version is auto-read from TELINK_VERSION
# If the file already exists, a _2, _3, ... suffix is appended
python3 build_and_update_notes.py --skip-build --skip-firmware
```

Example output:
```
Auto-detected version from TELINK_VERSION: tl_v1.0.1
File release-notes-tl_v1.0.1.md already exists, using tl_v1.0.1_2 instead
Generating release-notes-tl_v1.0.1_2.md from template release-notes-template.md...
```

### 5. Specify Output Version

```bash
# Generate release-notes-tl_v1.0.2.md (from release-notes-template.md template)
python3 build_and_update_notes.py --release-version tl_v1.0.2
```

When the target version is explicitly specified via `--release-version`, the script will:
1. Read the template file `release-notes-template.md`
2. Replace the `{{VERSION}}` placeholder in the template with the target version (e.g. `tl_v1.0.2`, including badge and tag name)
3. Replace the Resource Usage section with the latest build data
4. Write the output file `release-notes-<release-version>.md`

Can be combined with other options:

```bash
python3 build_and_update_notes.py --skip-build --release-version tl_v1.0.2
```


## Firmware Archive Contents

Each board's archive contains:
- `board_name/sample_name/zephyr.bin` - Binary firmware file
- `board_name/sample_name/zephyr.elf` - ELF format firmware file
- `board_name/sample_name/zephyr.hex` - Intel HEX format firmware file (if available)
- `board_name/sample_name/zephyr.dts` - Device tree file
- `board_name/sample_name/.config` - Configuration file

## Supported Boards

1. tlsr9518adk80d (TLSR951X/B91)
2. tlsr9528a (TLSR952X/B92)
3. tl3218x (TL321X)
4. tl3228x (TL322X)
5. tl3238x (TL323X)
6. tl7218x (TL721X)
7. tlsr9118bdk40d (TLSR9118BDK40D)

## Script Features

- Structured class design, easy to maintain and extend
- Template-based release notes generation: the template file (`release-notes-template.md`) is separate from the output file; the template is never modified
- The template uses `{{VERSION}}` as a placeholder; the script automatically replaces it with the target version (badge, tag name, etc.)
- **Version auto-detection**: when `--release-version` is not specified, the version is read from `zephyr/TELINK_VERSION`
- **Filename conflict handling**: if the output file already exists, a `_2`, `_3`, ... suffix is automatically appended
- Resource Usage section uses table format for clear display of memory usage per board per sample
- Only the Resource Usage section is replaced; all other static content is preserved
- Supports adding new boards and samples
- Detailed log output
- Build step can be skipped to generate directly from existing logs
- Automatically collects firmware files and creates a separate archive per board
- Supports skipping firmware collection and packaging

## Adding New Boards or Samples

In the `TelinkBuildManager` class:

1. Add new build configurations in the `_get_board_builds()` method
2. Add new sample name mappings in `sample_map` (if needed)
3. Add new board family mappings in `board_family`
4. Add new board ordering in `board_order`

## Updating the Template File

If you need to update static content in the release notes (such as Introduction, Version Information, Additional Notes, etc.), simply edit the **template file** `release-notes-template.md` (in the same directory as this script). The template uses `{{VERSION}}` as the version placeholder; the script automatically replaces it with the version specified via `--release-version` or auto-detected from `TELINK_VERSION` when generating the output file. The template itself remains unchanged.
