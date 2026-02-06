/*{
    "DESCRIPTION": "Gravitational lensing pinch effect on galaxy image",
    "CREDIT": "metalshade",
    "INPUTS": [
        {
            "NAME": "galaxy",
            "TYPE": "image"
        },
        {
            "NAME": "strength",
            "TYPE": "float",
            "DEFAULT": 0.5,
            "MIN": 0.0,
            "MAX": 2.0
        }
    ]
}*/

#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;

    // Center point for gravitational lens
    vec2 center = vec2(0.5, 0.5);
    vec2 toCenter = uv - center;
    float dist = length(toCenter);

    // Gravitational lensing - pinch effect
    // Objects closer to the center get pulled inward
    float lensStrength = 0.4 / (dist + 0.2);
    float pinch = 1.0 - lensStrength * smoothstep(0.0, 0.7, dist);

    // Apply radial pinch
    vec2 warpedUV = center + toCenter * pinch;

    // Sample texture
    vec4 color = texture(iChannel0, warpedUV);

    fragColor = color;
}
