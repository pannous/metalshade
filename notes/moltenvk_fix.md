# MoltenVK Library Conflict Fix

## Problem
Duplicate MoltenVK libraries cause runtime failures:
- Homebrew version: `/opt/homebrew/Cellar/molten-vk/1.4.0/lib/libMoltenVK.dylib`
- Old manual install: `/usr/local/lib/libMoltenVK.dylib` (CONFLICT)

## Permanent Fix (Recommended)

Run this command to remove the conflicting libraries:

```bash
sudo rm -f /usr/local/lib/libMoltenVK*
```

Or run the provided script:

```bash
./fix_moltenvk.sh
```

## Temporary Workaround

Use the wrapper script that sets correct environment variables:

```bash
./run_shader.sh shaders/organic_life.frag
```

This script sets:
- `VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json`
- `DYLD_LIBRARY_PATH` to prefer Homebrew version

## Verification

After removing duplicates, verify with:

```bash
ls /usr/local/lib/libMoltenVK* 2>/dev/null
# Should show: No such file or directory

ls /opt/homebrew/Cellar/molten-vk/*/lib/libMoltenVK.dylib
# Should show the Homebrew version only
```

## Test

After fix, run:

```bash
./metalshade shaders/organic_life.frag
```

Should see:
```
✓ GLFW window created
✓ Vulkan instance created (MoltenVK)
✓ Window surface created        # This should work now!
✓ Created ping-pong feedback buffers for paint effects
```
