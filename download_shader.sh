#!/bin/bash
# Download and convert a ShaderToy shader by ID or URL
# Usage: ./download_shader.sh <url_or_id> [output_name]
# Example: ./download_shader.sh https://www.shadertoy.com/view/4l2XWK

set -e

INPUT="$1"
OUTPUT_NAME="$2"

if [ -z "$INPUT" ]; then
    echo "Usage: $0 <url_or_id> [output_name]"
    echo ""
    echo "Examples:"
    echo "  $0 https://www.shadertoy.com/view/4l2XWK"
    echo "  $0 4l2XWK bumped_sinusoidal_warp"
    echo "  $0 XsXXDn seascape"
    exit 1
fi

# Extract shader ID from URL if provided
if [[ "$INPUT" =~ /view/([a-zA-Z0-9]+) ]]; then
    SHADER_ID="${BASH_REMATCH[1]}"
    echo "Extracted shader ID: $SHADER_ID"
else
    SHADER_ID="$INPUT"
fi

# Auto-generate output name if not provided
if [ -z "$OUTPUT_NAME" ]; then
    OUTPUT_NAME="shader_${SHADER_ID}"
fi

echo "Downloading ShaderToy shader: $SHADER_ID"
echo "Output name: $OUTPUT_NAME"

# Create shaders directory if it doesn't exist
mkdir -p shaders

# Download shader JSON from ShaderToy API
API_URL="https://www.shadertoy.com/api/v1/shaders/${SHADER_ID}?key=NtHtMm8j"
JSON_FILE="shaders/${OUTPUT_NAME}.json"

curl -s -A "metalshade" "$API_URL" -o "$JSON_FILE"

if [ ! -s "$JSON_FILE" ]; then
    echo "Error: Failed to download shader. Check shader ID."
    rm -f "$JSON_FILE"
    exit 1
fi

# Extract shader code using jq (if available) or simple grep
if command -v jq &> /dev/null; then
    SHADER_CODE=$(jq -r '.Shader.renderpass[0].code' "$JSON_FILE")
    SHADER_NAME=$(jq -r '.Shader.info.name' "$JSON_FILE")

    # Auto-use shader name as output if not specified by user
    if [ "$OUTPUT_NAME" == "shader_${SHADER_ID}" ] && [ "$SHADER_NAME" != "null" ]; then
        # Sanitize shader name for filename
        SANITIZED_NAME=$(echo "$SHADER_NAME" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/_/g' | sed 's/__*/_/g' | sed 's/^_//;s/_$//')
        if [ -n "$SANITIZED_NAME" ]; then
            OUTPUT_NAME="$SANITIZED_NAME"
        fi
    fi

    echo "✓ Downloaded shader: $SHADER_NAME"
    echo "✓ Output name: $OUTPUT_NAME"
else
    # Fallback: extract code between "code":" and the next "
    SHADER_CODE=$(grep -oP '"code"\s*:\s*"\K[^"]*' "$JSON_FILE" | head -1)
    echo "✓ Downloaded shader (install jq for better parsing)"
    echo "✓ Output name: $OUTPUT_NAME"
fi

# Rename JSON file if output name changed
if [ "$OUTPUT_NAME" != "shader_${SHADER_ID}" ]; then
    mv "$JSON_FILE" "shaders/${OUTPUT_NAME}.json"
    JSON_FILE="shaders/${OUTPUT_NAME}.json"
fi

# Save raw GLSL code
RAW_FILE="shaders/${OUTPUT_NAME}_raw.glsl"
echo "$SHADER_CODE" | sed 's/\\n/\n/g' | sed 's/\\t/\t/g' | sed 's/\\"/"/g' > "$RAW_FILE"

echo "✓ Saved raw GLSL to: $RAW_FILE"
echo ""
echo "Next steps:"
echo "1. Convert the shader to Vulkan GLSL format"
echo "2. Replace 'void mainImage(...)' with Vulkan setup"
echo "3. Add uniform buffer declarations"
echo "4. Update iTime, iResolution references to ubo.*"
echo "5. Compile to SPIR-V and test"
echo ""
echo "See HOWTO.md for detailed conversion instructions"
