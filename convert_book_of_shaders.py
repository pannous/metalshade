#!/usr/bin/env python3
"""Convert Book of Shaders format to Vulkan GLSL"""
import sys
import re

input_file = sys.argv[1]
output_file = sys.argv[2]

with open(input_file, 'r') as f:
    shader = f.read()

# Remove Book of Shaders specific headers
shader = re.sub(r'#ifdef GL_ES\s+precision mediump float;\s+#endif', '', shader, flags=re.MULTILINE)

# Remove old uniform declarations
shader = re.sub(r'^\s*uniform\s+vec2\s+u_resolution\s*;', '', shader, flags=re.MULTILINE)
shader = re.sub(r'^\s*uniform\s+vec2\s+u_mouse\s*;', '', shader, flags=re.MULTILINE)
shader = re.sub(r'^\s*uniform\s+float\s+u_time\s*;', '', shader, flags=re.MULTILINE)

# Replace uniform references (word boundaries to avoid replacing variables)
shader = re.sub(r'\bu_time\b', 'ubo.iTime', shader)
shader = re.sub(r'\bu_resolution\b', 'ubo.iResolution.xy', shader)
shader = re.sub(r'\bu_mouse\b', 'ubo.iMouse.xy', shader)

# Replace gl_FragCoord and gl_FragColor
shader = re.sub(r'\bgl_FragCoord\b', 'fragCoord', shader)
shader = re.sub(r'\bgl_FragColor\b', 'fragColor', shader)

# Add Vulkan header
vulkan_header = """#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;

"""

shader = vulkan_header + shader

with open(output_file, 'w') as f:
    f.write(shader)

print(f"✓ Converted: {input_file} → {output_file}")
