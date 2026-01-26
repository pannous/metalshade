#!/usr/bin/env python3
"""
Automatically convert ShaderToy GLSL to Vulkan GLSL format
Usage: ./convert_shader.py <input_file> [output_file]
"""

import sys
import re
from pathlib import Path


def convert_shadertoy_to_vulkan(code):
    """Convert ShaderToy GLSL to Vulkan-compatible GLSL"""

    # Remove ShaderToy-specific comments and URLs
    code = re.sub(r'// http://www\.pouet\.net.*\n', '', code)

    # Add Vulkan version header
    vulkan_header = """#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;
// layout(binding = 2) uniform sampler2D iChannel1;
// layout(binding = 3) uniform sampler2D iChannel2;
// layout(binding = 4) uniform sampler2D iChannel3;

"""

    # Replace common ShaderToy defines
    code = re.sub(r'#define\s+t\s+iTime', '', code)
    code = re.sub(r'#define\s+r\s+iResolution\.xy', '', code)

    # Replace uniform references with ubo. prefix
    # Important: Use negative lookbehind to avoid double-replacement
    code = re.sub(r'(?<!ubo\.)\biTime\b', 'ubo.iTime', code)
    code = re.sub(r'(?<!ubo\.)\biResolution\b', 'ubo.iResolution', code)
    code = re.sub(r'(?<!ubo\.)\biMouse\b', 'ubo.iMouse', code)

    # Replace other common uniforms
    code = re.sub(r'\biTimeDelta\b', '0.016', code)  # Approximate 60fps
    code = re.sub(r'\biFrame\b', 'int(ubo.iTime * 60.0)', code)
    code = re.sub(r'\biFrameRate\b', '60.0', code)
    code = re.sub(r'\biDate\b', 'vec4(2024.0, 1.0, 1.0, 0.0)', code)
    code = re.sub(r'\biSampleRate\b', '44100.0', code)

    # Handle texture() calls for channels
    # Already correct in ShaderToy format

    # Replace mainImage signature with main
    # Pattern: void mainImage( out vec4 fragColor, in vec2 fragCoord )
    code = re.sub(
        r'void\s+mainImage\s*\(\s*out\s+vec4\s+\w+\s*,\s*in\s+vec2\s+\w+\s*\)',
        'void main()',
        code
    )

    # Expand common #define shortcuts used in code-golfed shaders
    # Replace 't' when it's standalone (common iTime shortcut)
    code = re.sub(r'(?<![a-zA-Z_])\bt\b(?![a-zA-Z_])', 'ubo.iTime', code)

    # Replace 'r' when used as iResolution.xy shortcut
    # Match 'r' when NOT part of a longer word (return, for, true, etc.)
    # This is tricky - we want to match /r and r.x and r; but not return, for, etc.
    code = re.sub(r'(?<![a-zA-Z_])\br\b(?![a-zA-Z_])', 'ubo.iResolution.xy', code)

    # Clean up multiple blank lines
    code = re.sub(r'\n\n\n+', '\n\n', code)

    # Combine header with converted code
    result = vulkan_header + code.strip() + '\n'

    return result


def sanitize_filename(name):
    """Convert to valid shader filename"""
    name = re.sub(r'[^\w\s-]', '', name.lower())
    name = re.sub(r'[-\s]+', '_', name)
    return name.strip('_')


def main():
    if len(sys.argv) < 2:
        print("Usage: ./convert_shader.py <input_file> [output_file]")
        print()
        print("Examples:")
        print("  ./convert_shader.py shaders/creation_by_silexars_raw.glsl")
        print("  ./convert_shader.py shaders/bumped_sinusoidal_warp_raw.glsl shaders/bumped.frag")
        print()
        print("Converts ShaderToy GLSL to Vulkan GLSL format with:")
        print("  - Vulkan #version 450 header")
        print("  - Uniform buffer object declarations")
        print("  - main() instead of mainImage()")
        print("  - ubo.iTime, ubo.iResolution, ubo.iMouse prefixes")
        sys.exit(1)

    input_file = Path(sys.argv[1])

    if not input_file.exists():
        print(f"Error: Input file not found: {input_file}")
        sys.exit(1)

    # Determine output filename
    if len(sys.argv) > 2:
        output_file = Path(sys.argv[2])
    else:
        # Auto-generate: remove _raw suffix, change to .frag
        stem = input_file.stem.replace('_raw', '')
        output_file = input_file.parent / f"{stem}.frag"

    print(f"Converting: {input_file}")
    print(f"Output:     {output_file}")

    # Read input
    try:
        with open(input_file, 'r') as f:
            shadertoy_code = f.read()
    except Exception as e:
        print(f"Error reading file: {e}")
        sys.exit(1)

    # Convert
    vulkan_code = convert_shadertoy_to_vulkan(shadertoy_code)

    # Write output
    try:
        with open(output_file, 'w') as f:
            f.write(vulkan_code)
        print(f"✓ Converted successfully!")
        print(f"✓ Saved to: {output_file}")
    except Exception as e:
        print(f"Error writing file: {e}")
        sys.exit(1)

    # Provide next steps
    print()
    print("Next steps:")
    print(f"1. Review the converted shader: cat {output_file}")
    print(f"2. Compile: glslangValidator -V {output_file} -o frag.spv")
    print(f"3. Test: cp {output_file} shadertoy.frag && make && ./run.sh")
    print()
    print("Note: Some shaders may need manual adjustments for:")
    print("  - Multiple render passes (Buffer A, B, C, etc.)")
    print("  - Additional texture channels (iChannel1-3)")
    print("  - Complex uniform variables")


if __name__ == "__main__":
    main()
