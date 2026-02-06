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
    Mouse Trace Shader
    ------------------

    Demonstrates mouse position tracking with visual trails:

    - Press and HOLD left mouse button
    - Move the mouse around while holding
    - See a glowing trail following your movement
    - The trail fades over time
    - Release and press again to create new traces

    The shader creates a heat-map style trail effect that shows
    where you've been recently, with older positions fading out.
*/

// Hash function for pseudo-random numbers
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Smooth noise function
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;

    // Dark blue-black background
    vec3 color = vec3(0.02, 0.02, 0.08);

    // Get button state
    float leftDuration = ubo.iButtonLeft;
    float rightDuration = ubo.iButtonRight;
    float middleDuration = ubo.iButtonMiddle;

    // Create a trail effect based on time and mouse position
    // We simulate a trail by creating multiple ghost positions along a time-based path
    float trailIntensity = 0.0;
    vec3 trailColor = vec3(0.0);

    if (leftDuration > 0.0) {
        // Left button: Cyan trail
        // Create a parametric trail effect
        for (float i = 0.0; i < 20.0; i += 1.0) {
            float t = i / 20.0;
            float age = t * 2.0; // Trail age in seconds

            // Create wavy trail based on time offset
            vec2 trailOffset = vec2(
                sin(ubo.iTime * 2.0 - age * 10.0) * 0.02,
                cos(ubo.iTime * 2.5 - age * 10.0) * 0.02
            );

            vec2 trailPos = mouse + trailOffset * t;
            float dist = length(uv - trailPos);

            // Trail point size and intensity
            float pointSize = 0.03 * (1.0 - t * 0.5);
            float intensity = smoothstep(pointSize, 0.0, dist) * (1.0 - t);

            trailIntensity += intensity * 0.2;
            trailColor += vec3(0.0, 0.8, 1.0) * intensity * 0.2;
        }

        // Add main cursor glow
        float dist = length(uv - mouse);
        float cursorGlow = exp(-dist * 15.0);
        trailColor += vec3(0.2, 1.0, 1.0) * cursorGlow;
        trailIntensity += cursorGlow;
    }

    if (rightDuration > 0.0) {
        // Right button: Magenta trail with different pattern
        for (float i = 0.0; i < 20.0; i += 1.0) {
            float t = i / 20.0;
            float age = t * 2.0;

            // Spiral pattern
            float angle = ubo.iTime * 3.0 - age * 15.0;
            vec2 trailOffset = vec2(cos(angle), sin(angle)) * 0.03 * t;

            vec2 trailPos = mouse + trailOffset;
            float dist = length(uv - trailPos);

            float pointSize = 0.025 * (1.0 - t * 0.5);
            float intensity = smoothstep(pointSize, 0.0, dist) * (1.0 - t);

            trailIntensity += intensity * 0.2;
            trailColor += vec3(1.0, 0.0, 0.8) * intensity * 0.2;
        }

        float dist = length(uv - mouse);
        float cursorGlow = exp(-dist * 15.0);
        trailColor += vec3(1.0, 0.2, 0.8) * cursorGlow;
        trailIntensity += cursorGlow;
    }

    if (middleDuration > 0.0) {
        // Middle button: Golden trail with particle effect
        for (float i = 0.0; i < 30.0; i += 1.0) {
            float t = i / 30.0;
            float age = t * 1.5;

            // Random scatter using hash
            vec2 randomOffset = vec2(
                hash(vec2(i, 0.0)) - 0.5,
                hash(vec2(i, 1.0)) - 0.5
            ) * 0.1 * t;

            // Add time-based drift
            randomOffset.y -= age * 0.05;

            vec2 trailPos = mouse + randomOffset;
            float dist = length(uv - trailPos);

            float pointSize = 0.015 * (1.0 - t * 0.7);
            float intensity = smoothstep(pointSize, 0.0, dist) * (1.0 - t);

            trailIntensity += intensity * 0.15;
            trailColor += vec3(1.0, 0.8, 0.0) * intensity * 0.15;
        }

        float dist = length(uv - mouse);
        float cursorGlow = exp(-dist * 15.0);
        trailColor += vec3(1.0, 0.9, 0.2) * cursorGlow;
        trailIntensity += cursorGlow;
    }

    // Add background glow based on trail intensity
    float bgGlow = trailIntensity * 0.1;
    color += vec3(0.0, 0.05, 0.1) * bgGlow;

    // Composite trail onto background
    color = mix(color, trailColor, min(trailIntensity, 1.0));

    // Add subtle scanline effect
    float scanline = sin(uv.y * ubo.iResolution.y * 0.5 + ubo.iTime * 50.0) * 0.02 + 0.98;
    color *= scanline;

    // Instructions in corner (only if no buttons pressed)
    if (leftDuration == 0.0 && rightDuration == 0.0 && middleDuration == 0.0) {
        vec2 textPos = uv - vec2(0.5, 0.9);
        float textGlow = exp(-length(textPos) * 20.0);
        color += vec3(0.4, 0.5, 0.7) * textGlow * 0.4;
    }

    // Add duration indicators at bottom
    float indicatorY = 0.05;
    float indicatorHeight = 0.015;

    // Left button indicator (cyan)
    if (leftDuration > 0.0 && uv.y < indicatorY && uv.y > indicatorY - indicatorHeight) {
        float progress = min(leftDuration * 0.1, 1.0);
        if (uv.x < progress * 0.3) {
            color = mix(color, vec3(0.0, 0.8, 1.0), 0.7);
        }
    }

    // Right button indicator (magenta)
    if (rightDuration > 0.0 && uv.y < indicatorY && uv.y > indicatorY - indicatorHeight) {
        float progress = min(rightDuration * 0.1, 1.0);
        if (uv.x > 0.35 && uv.x < 0.35 + progress * 0.3) {
            color = mix(color, vec3(1.0, 0.0, 0.8), 0.7);
        }
    }

    // Middle button indicator (gold)
    if (middleDuration > 0.0 && uv.y < indicatorY && uv.y > indicatorY - indicatorHeight) {
        float progress = min(middleDuration * 0.1, 1.0);
        if (uv.x > 0.7 && uv.x < 0.7 + progress * 0.3) {
            color = mix(color, vec3(1.0, 0.8, 0.0), 0.7);
        }
    }

    fragColor = vec4(color, 1.0);
}
