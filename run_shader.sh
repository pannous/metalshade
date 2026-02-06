#!/bin/bash
# Wrapper script to run shaders with correct MoltenVK configuration

# Force Homebrew MoltenVK (prevents conflict with /usr/local/lib version)
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/molten-vk/1.4.0/lib:$DYLD_LIBRARY_PATH

# Run the shader viewer
./metalshade "$@"
