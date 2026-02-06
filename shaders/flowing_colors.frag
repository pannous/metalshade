#version 450

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

/*
    Flowing Colors - Interactive Wave Field
    ----------------------------------------

    A mesmerizing, continuously evolving color field that responds to mouse input.
    Creates organic, flowing patterns without needing feedback buffers.

    Controls:
    - Move mouse to influence the flow
    - Left click: Add energy waves
    - Right click: Create vortex
    - Middle click: Turbulence burst
    - Button 4/5: Change wave patterns
*/

// Hash for pseudo-random
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1, 0));
    float c = hash(i + vec2(0, 1));
    float d = hash(i + vec2(1, 1));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractional Brownian Motion
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;

    // Distance and angle to mouse
    vec2 toMouse = uv - mouse;
    float distToMouse = length(toMouse);
    float angleToMouse = atan(toMouse.y, toMouse.x);

    // Base flow field
    vec2 flow = uv * 4.0 + ubo.iTime * 0.3;

    // Add swirling motion around mouse
    float swirl = exp(-distToMouse * 5.0);
    flow += vec2(
        sin(angleToMouse + ubo.iTime) * swirl,
        cos(angleToMouse + ubo.iTime) * swirl
    );

    // Mouse interactions
    if (ubo.iButtonLeft > 0.0) {
        // Rippling waves from mouse
        float ripple = sin(distToMouse * 20.0 - ubo.iTime * 5.0);
        flow += toMouse * ripple * 0.5;
    }

    if (ubo.iButtonRight > 0.0) {
        // Vortex effect
        float rotation = ubo.iTime * 0.03;
        mat2 rot = mat2(cos(rotation), -sin(rotation), sin(rotation), cos(rotation));
        flow = rot * (flow - vec2(mouse)) + vec2(mouse);
    }

    if (ubo.iButtonMiddle > 0.0) {
        // Turbulence
        flow += vec2(
            fbm(uv * 10.0 + ubo.iTime),
            fbm(uv * 10.0 + ubo.iTime + 100.0)
        ) * 2.0;
    }

    // Generate flowing colors using fbm
    float n1 = fbm(flow);
    float n2 = fbm(flow + vec2(5.2, 1.3));
    float n3 = fbm(flow + vec2(2.8, 9.1));

    // Color mapping with smooth gradients
    vec3 color1 = vec3(0.6, 0.2, 0.8);  // Purple
    vec3 color2 = vec3(0.2, 0.8, 0.9);  // Cyan
    vec3 color3 = vec3(0.9, 0.4, 0.2);  // Orange

    vec3 color = mix(color1, color2, n1);
    color = mix(color, color3, n2);

    // Add glow highlights
    float highlight = pow(n3, 3.0);
    color += vec3(1.0, 0.9, 0.7) * highlight * 0.5;

    // Add cursor glow
    float cursor = exp(-distToMouse * 15.0);
    color += vec3(cursor * 0.5);

    // Subtle vignette
    float vignette = 1.0 - length(uv - 0.5) * 0.5;
    color *= vignette;

    // Button-specific effects
    if (ubo.iButton4 > 0.0) {
        // Rainbow shift
        float rainbow = sin(ubo.iTime * 2.0);
        color = mix(color, color.gbr, rainbow * 0.5 + 0.5);
    }

    if (ubo.iButton5 > 0.0) {
        // High contrast mode
        color = pow(color, vec3(2.0));
    }

    fragColor = vec4(color, 1.0);
}
