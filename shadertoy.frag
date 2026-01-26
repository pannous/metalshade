#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

// Classic tunnel effect
void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= ubo.iResolution.x / ubo.iResolution.y;

    float time = ubo.iTime;

    float r = length(uv);
    float a = atan(uv.y, uv.x);

    float u = 1.0 / r + time * 0.3;
    float v = a / 3.14159;

    vec3 col = 0.5 + 0.5 * cos(u * 2.0 + vec3(0, 2, 4));
    col *= 1.0 - exp(-r * 0.5);

    fragColor = vec4(col, 1.0);
}
