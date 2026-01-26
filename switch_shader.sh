#!/bin/bash
# Switch to a different shader example
# Usage: ./switch_shader.sh <shader_name>

SHADER_NAME="$1"

if [ -z "$SHADER_NAME" ]; then
    echo "Available shaders:"
    echo ""
    ls -1 examples/*.frag 2>/dev/null | sed 's/examples\//  /' | sed 's/\.frag$//' || echo "  No example shaders found"
    ls -1 shaders/*.frag 2>/dev/null | sed 's/shaders\//  /' | sed 's/\.frag$//' || true
    echo "  shadertoy (default - bumped sinusoidal warp)"
    echo ""
    echo "Usage: $0 <shader_name>"
    echo ""
    echo "Examples:"
    echo "  $0 simple_gradient"
    echo "  $0 tunnel"
    echo "  $0 plasma"
    echo "  $0 shadertoy"
    exit 1
fi

# Check if shader exists
SHADER_FILE=""

if [ "$SHADER_NAME" = "shadertoy" ]; then
    SHADER_FILE="shadertoy.frag"
elif [ -f "examples/${SHADER_NAME}.frag" ]; then
    SHADER_FILE="examples/${SHADER_NAME}.frag"
elif [ -f "shaders/${SHADER_NAME}.frag" ]; then
    SHADER_FILE="shaders/${SHADER_NAME}.frag"
else
    echo "Error: Shader '$SHADER_NAME' not found"
    exit 1
fi

echo "Switching to shader: $SHADER_NAME"
echo "Source: $SHADER_FILE"

# Copy to working location
cp "$SHADER_FILE" shadertoy.frag

# Recompile
echo "Compiling shader..."
glslangValidator -V shadertoy.frag -o frag.spv

if [ $? -eq 0 ]; then
    echo "✓ Shader compiled successfully!"
    echo "Run ./run.sh to view"
else
    echo "✗ Shader compilation failed"
    exit 1
fi
