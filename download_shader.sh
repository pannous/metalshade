#!/bin/bash
# Download and convert a ShaderToy shader by ID
# Usage: ./download_shader.sh <shader_id> [output_name]
# Example: ./download_shader.sh 4l2XWK bumped_sinusoidal_warp

set -e

SHADER_ID="$1"
OUTPUT_NAME="${2:-shader_${SHADER_ID}}"

if [ -z "$SHADER_ID" ]; then
    echo "Usage: $0 <shader_id> [output_name]"
    echo ""
    echo "Examples:"
    echo "  $0 4l2XWK bumped_sinusoidal_warp"
    echo "  $0 XsXXDn seascape"
    echo "  $0 4dX3Rn simple_fire"
    echo ""
    echo "Find shader IDs from ShaderToy URLs:"
    echo "  https://www.shadertoy.com/view/4l2XWK"
    echo "                                ^^^^^^"
    exit 1
fi

echo "Downloading ShaderToy shader: $SHADER_ID"
echo "Output name: $OUTPUT_NAME"

# Create shaders directory if it doesn't exist
mkdir -p shaders

# Download shader JSON from ShaderToy API
API_URL="https://www.shadertoy.com/api/v1/shaders/${SHADER_ID}?key=NtHtMm8j"
JSON_FILE="shaders/${OUTPUT_NAME}.json"

curl -s "$API_URL" -o "$JSON_FILE"

if [ ! -s "$JSON_FILE" ]; then
    echo "Error: Failed to download shader. Check shader ID."
    rm -f "$JSON_FILE"
    exit 1
fi

# Extract shader code using jq (if available) or simple grep
if command -v jq &> /dev/null; then
    SHADER_CODE=$(jq -r '.Shader.renderpass[0].code' "$JSON_FILE")
    SHADER_NAME=$(jq -r '.Shader.info.name' "$JSON_FILE")
    echo "✓ Downloaded shader: $SHADER_NAME"
else
    # Fallback: extract code between "code":" and the next "
    SHADER_CODE=$(grep -oP '"code"\s*:\s*"\K[^"]*' "$JSON_FILE" | head -1)
    echo "✓ Downloaded shader (install jq for better parsing)"
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
