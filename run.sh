#!/bin/bash
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
export DYLD_LIBRARY_PATH=/opt/homebrew/lib:$DYLD_LIBRARY_PATH
export MVK_CONFIG_LOG_LEVEL=1

echo "Starting ShaderToy Viewer with Vulkan + MoltenVK..."
./shadertoy_viewer
