#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

// Simple animated gradient - perfect for testing
void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;

    // Animated colors
    float time = ubo.iTime;
    vec3 col = 0.5 + 0.5 * cos(time + uv.xyx + vec3(0, 2, 4));

    fragColor = vec4(col, 1.0);
}
