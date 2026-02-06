#!/usr/bin/env python3
"""
Convert code-golf / minified ShaderToy shaders (from Twitter, etc.)
These use implicit variables and are missing the mainImage wrapper.

Common patterns:
- FC = fragCoord (or gl_FragCoord)
- r = iResolution
- t = iTime
- o = fragColor (output)
- m = iMouse
"""
import sys
import re

if len(sys.argv) < 2:
    print("Usage: convert_golf.py <input.raw> [output.frag]")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2] if len(sys.argv) > 2 else input_file.replace('.raw', '.frag')

with open(input_file, 'r') as f:
    shader = f.read().strip()

# Detect common code golf variable patterns
has_FC = 'FC' in shader or 'gl_FragCoord' in shader
has_r = re.search(r'\br\b', shader)
has_t = re.search(r'\bt\b', shader)
has_o = re.search(r'\bo\b', shader)
has_m = re.search(r'\bm\b', shader)

print(f"📦 Code golf shader detected:")
print(f"   FC (fragCoord): {has_FC}")
print(f"   r (iResolution): {bool(has_r)}")
print(f"   t (iTime): {bool(has_t)}")
print(f"   o (fragColor): {bool(has_o)}")
print(f"   m (iMouse): {bool(has_m)}")

# Build the wrapper with necessary variable declarations
wrapper_start = """#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
    vec2 iScroll;
    float iButtonLeft;
    float iButtonRight;
    float iButtonMiddle;
    float iButton4;
    float iButton5;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;
layout(binding = 2) uniform sampler2D iChannel1;

/*
    Code Golf Shader (Twitter/Pouet style)
    Converted from compact format
*/

void main() {
"""

# Add variable declarations based on what's used
declarations = []
if has_FC:
    declarations.append("    vec2 FC = fragCoord;  // Short alias")
if has_r:
    declarations.append("    vec3 r = ubo.iResolution;  // Resolution")
if has_t:
    declarations.append("    float t = ubo.iTime;  // Time")
if has_o:
    declarations.append("    vec4 o = vec4(0.0);  // Output color")
if has_m:
    declarations.append("    vec4 m = ubo.iMouse;  // Mouse")

wrapper_vars = "\n".join(declarations)

# The shader body (as-is, might need cleanup)
shader_body = "    " + shader.replace('\n', '\n    ')

# Add output assignment if 'o' is used
wrapper_end = ""
if has_o:
    wrapper_end = "\n    fragColor = o;"
wrapper_end += "\n}\n"

# Combine everything
converted = wrapper_start + wrapper_vars + "\n\n" + shader_body + wrapper_end

# Try to fix common issues
# Replace gl_FragCoord with FC if it wasn't already done
converted = re.sub(r'\bgl_FragCoord\b', 'FC', converted)

# Replace texture2D with texture
converted = re.sub(r'\btexture2D\b', 'texture', converted)

# Write output
with open(output_file, 'w') as f:
    f.write(converted)

print(f"✓ Converted: {input_file} → {output_file}")
print(f"\nTest with: ./metalshade {output_file}")
print(f"\n⚠️  IMPORTANT: Code golf shaders often need manual fixes!")
print(f"    Common issues:")
print(f"    - Division by zero (check for f=0 in loops)")
print(f"    - Broken math (o*0 = always black!)")
print(f"    - Missing semicolons")
print(f"    - Unusual loop syntax")
print(f"\n💡 If it shows black, check for:")
print(f"    - o*0 or similar (multiplying output by zero)")
print(f"    - Division by zero in loop counters")
print(f"    - NaN from invalid operations")
print(f"\nSee twitter_example.frag for a working reference!")
