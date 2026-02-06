#version 450
// @texture galaxy.jpg

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;

    // Center point for distortion
    vec2 center = vec2(0.5, 0.5);
    vec2 toCenter = uv - center;

    // Distance from center
    float dist = length(toCenter);

    // Angle for spiral effect
    float angle = atan(toCenter.y, toCenter.x);

    // Create rotating spiral twist based on distance
    // Closer to center = more twist
    float twistAmount = 0.6 / (dist + 0.3);
    float rotation = twistAmount * (1.0 - smoothstep(0.0, 0.8, dist));

    // Add time-based rotation (subtle)
    rotation += ubo.iTime * 0.1;

    // Apply spiral rotation
    float newAngle = angle + rotation;

    // Radial compression towards center (black hole pull)
    float compression = 1.0 - exp(-dist * 2.0);
    float warpedDist = dist * compression;

    // Additional subtle non-linear warp
    warpedDist = pow(warpedDist, 0.85 + 0.15 * sin(ubo.iTime * 0.3));

    // Reconstruct warped UV coordinates
    vec2 warpedUV = center + vec2(cos(newAngle), sin(newAngle)) * warpedDist;

    // Sample the texture with warped coordinates
    vec4 color = texture(iChannel0, warpedUV);

    fragColor = color;
}
