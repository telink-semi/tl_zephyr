# Patch List

## openthread_csma_backoff.patch

### Purpose
This patch modifies the OpenThread stack to skip the initial CSMA-CA backoff when sending data requests. When used with the power-saving mode, it can help reduce power consumption in Thread mode.

### How to Apply

1. Navigate to the OpenThread module directory:

```bash
cd zephyrproject/modules/lib/openthread
```

2. Apply the patch file:

```bash
git apply ../../../zephyr/telink_patches/openthread_csma_backoff.patch
```

3. Rebuild your Zephyr application to ensure the patched code is included.
