#!/bin/bash
# Fix MoltenVK library conflict

echo "Removing duplicate MoltenVK libraries from /usr/local/lib..."
sudo rm -f /usr/local/lib/libMoltenVK*

echo "Verifying removal..."
if [ ! -f /usr/local/lib/libMoltenVK.dylib ]; then
    echo "✓ Successfully removed conflicting MoltenVK libraries"
else
    echo "✗ Failed to remove libraries - check permissions"
    exit 1
fi

echo "Checking Homebrew MoltenVK..."
ls -la /opt/homebrew/Cellar/molten-vk/*/lib/libMoltenVK.dylib

echo ""
echo "✓ MoltenVK conflict resolved!"
echo "Now run: ./metalshade shaders/organic_life.frag"
