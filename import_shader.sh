#!/bin/bash
# Fully automatic ShaderToy to Vulkan shader importer
# Fetches, converts, and compiles a shader in one command
# Usage: ./import_shader.sh <url_or_id> [name]

set -e

URL_OR_ID="$1"
OUTPUT_NAME="$2"

if [ -z "$URL_OR_ID" ]; then
    echo "Automatic ShaderToy Shader Importer"
    echo "===================================="
    echo ""
    echo "Fetches, converts, and compiles ShaderToy shaders automatically!"
    echo ""
    echo "Usage: $0 <url_or_id> [output_name]"
    echo ""
    echo "Examples:"
    echo "  $0 https://www.shadertoy.com/view/XsXXDn"
    echo "  $0 4l2XWK bumped_warp"
    echo "  $0 MdlXz8  # Auto-detects shader name"
    echo ""
    echo "This script:"
    echo "  1. Fetches shader from ShaderToy (requires Claude Code + Playwright)"
    echo "  2. Converts ShaderToy GLSL → Vulkan GLSL"
    echo "  3. Compiles to SPIR-V"
    echo "  4. Ready to run!"
    exit 1
fi

echo "╔══════════════════════════════════════════════════════════╗"
echo "║    Automatic ShaderToy to Vulkan Shader Importer        ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Inform user about browser automation requirement
echo "This script requires Claude Code with Playwright browser automation."
echo "Please run these commands in Claude Code:"
echo ""
echo "  1. Fetch shader:"
if [ -n "$OUTPUT_NAME" ]; then
    echo "     /fetch-shader $URL_OR_ID $OUTPUT_NAME"
else
    echo "     /fetch-shader $URL_OR_ID"
fi
echo ""
echo "  2. Then run this import script again, OR:"
echo ""
echo "     Manually convert and compile:"

# Extract shader ID
if [[ "$URL_OR_ID" =~ /view/([a-zA-Z0-9]+) ]]; then
    SHADER_ID="${BASH_REMATCH[1]}"
else
    SHADER_ID="$URL_OR_ID"
fi

# Determine expected filenames
if [ -n "$OUTPUT_NAME" ]; then
    RAW_FILE="shaders/${OUTPUT_NAME}_raw.glsl"
    FRAG_FILE="shaders/${OUTPUT_NAME}.frag"
else
    # Look for any recently modified *_raw.glsl files
    RAW_FILE=$(ls -t shaders/*_raw.glsl 2>/dev/null | head -1)
    if [ -z "$RAW_FILE" ]; then
        echo ""
        echo "⚠️  No raw shader files found in shaders/"
        echo "    Please fetch a shader first using: /fetch-shader $URL_OR_ID"
        exit 1
    fi
    echo ""
    echo "⚠️  Shader ID not embedded in filenames."
    echo "    Using most recent raw shader: $RAW_FILE"
    echo "    (If this is wrong, specify output name explicitly)"
    echo ""
    FRAG_FILE="${RAW_FILE%_raw.glsl}.frag"
fi

# Check if raw file exists
if [ ! -f "$RAW_FILE" ]; then
    echo ""
    echo "⚠️  Raw shader file not found: $RAW_FILE"
    echo "    Please fetch it first using: /fetch-shader $URL_OR_ID"
    exit 1
fi

echo ""
echo "Found shader: $RAW_FILE"
echo ""
echo "Step 2: Converting to Vulkan GLSL and compiling to SPIR-V..."
if ! ./shadertoy2vulkan "$RAW_FILE" "$FRAG_FILE"; then
    echo "❌ Conversion/compilation failed!"
    exit 1
fi

# The shadertoy2vulkan tool creates both .frag and .frag.spv
# We need to copy the .frag.spv to frag.spv for the build system
SPV_FILE="${FRAG_FILE}.spv"
if [ -f "$SPV_FILE" ]; then
    cp "$SPV_FILE" frag.spv
    echo "✓ Copied SPIR-V to frag.spv"
fi
echo ""
echo "Step 4: Ready to run!"
echo ""
SHADER_NAME=$(basename "$FRAG_FILE" .frag)
echo "  ./switch_shader.sh $SHADER_NAME"
echo "  ./run.sh"
echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  Import complete! Shader ready for viewing.             ║"
echo "╚══════════════════════════════════════════════════════════╝"
