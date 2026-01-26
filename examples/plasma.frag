#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

// Plasma effect
void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    float time = ubo.iTime;

    vec2 p = uv * 8.0;

    float value = 0.0;
    value += sin(p.x + time);
    value += sin(p.y + time);
    value += sin(p.x + p.y + time);
    value += sin(sqrt(p.x * p.x + p.y * p.y) + time);
    value /= 4.0;

    vec3 col = 0.5 + 0.5 * cos(value * 3.14159 + vec3(0, 0.5, 1.0));

    fragColor = vec4(col, 1.0);
}
