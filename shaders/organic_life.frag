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

layout(binding = 1) uniform sampler2D iChannel0;
layout(binding = 2) uniform sampler2D iChannel1;  // Previous frame feedback

/*
    Organic Life - Continuous Feedback Demo
    ----------------------------------------

    A smooth, continuous cellular automaton inspired by reaction-diffusion
    systems. Creates flowing, organic patterns that evolve over time.

    Unlike pixelated Game of Life, this uses continuous mathematics
    and blur/diffusion to create smooth, liquid-like behaviors.

    Controls:
    - Left Click: Add "life" activator
    - Right Click: Reset to blank canvas
    - Middle Click: Add turbulence
    - Button 4: Reset to random state
    - Move mouse to influence flow

    The system uses:
    - Diffusion: Energy spreads to neighbors
    - Reaction: Local interactions create patterns
    - Decay: Slow fade prevents saturation
*/

// Sample with bilinear filtering for smooth diffusion
vec3 samplePrev(vec2 uv, vec2 offset) {
    vec2 pixel = 1.0 / ubo.iResolution.xy;
    return texture(iChannel1, uv + offset * pixel).rgb;
}

// Laplacian operator for diffusion (approximates heat equation)
vec3 laplacian(vec2 uv) {
    vec3 sum = vec3(0.0);

    // 3x3 kernel (wider gives more diffusion)
    sum += samplePrev(uv, vec2(-1, -1)) * 0.05;
    sum += samplePrev(uv, vec2( 0, -1)) * 0.2;
    sum += samplePrev(uv, vec2( 1, -1)) * 0.05;

    sum += samplePrev(uv, vec2(-1,  0)) * 0.2;
    sum += samplePrev(uv, vec2( 0,  0)) * -1.0;  // Center weight
    sum += samplePrev(uv, vec2( 1,  0)) * 0.2;

    sum += samplePrev(uv, vec2(-1,  1)) * 0.05;
    sum += samplePrev(uv, vec2( 0,  1)) * 0.2;
    sum += samplePrev(uv, vec2( 1,  1)) * 0.05;

    return sum;
}

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;

    // Read previous frame state
    vec3 prev = texture(iChannel1, uv).rgb;

    // Compute diffusion using Laplacian
    vec3 lap = laplacian(uv);

    // Diffusion rate (how fast energy spreads)
    float diffusionRate = 0.2;

    // Apply diffusion
    vec3 state = prev + lap * diffusionRate;

    // Reaction term: Create interesting dynamics
    // Red channel: Activator (grows and spreads)
    // Green channel: Inhibitor (suppresses growth)
    // Blue channel: Substrate (food source)

    float distToMouse = length(uv - mouse);

    // Reaction-diffusion rules (inspired by Gray-Scott model)
    float activator = state.r;
    float inhibitor = state.g;
    float substrate = state.b;

    // Local reactions
    float feed = 0.055;      // Feed rate (adds substrate)
    float kill = 0.062;      // Kill rate (removes inhibitor)

    // Autocatalytic reaction: A + 2B → 3B
    float reaction = activator * inhibitor * inhibitor;

    // Update rules
    activator += -reaction + feed * (1.0 - activator);
    inhibitor += reaction - (kill + feed) * inhibitor;
    substrate += -reaction * 0.1 + feed * 0.5;

    // Very slow decay to prevent saturation
    state = vec3(activator, inhibitor, substrate) * 0.998;

    // Mouse interactions
    if (ubo.iButtonLeft > 0.0) {
        // Add activator (red)
        float influence = exp(-distToMouse * 20.0);
        state.r += influence * 0.5;
        state.b += influence * 0.3;  // Add substrate too
    }

    if (ubo.iButtonRight > 0.0) {
        // Reset to black (clear canvas)
        state = vec3(0.0);
    }

    if (ubo.iButtonMiddle > 0.0) {
        // Add turbulence (all channels)
        float influence = exp(-distToMouse * 15.0);
        float noise = fract(sin(dot(uv + ubo.iTime * 0.1, vec2(12.9898, 78.233))) * 43758.5453);
        state += vec3(noise) * influence * 0.3;
    }

    if (ubo.iButton4 > 0.0) {
        // Reset: Random initial state
        float noise1 = fract(sin(dot(uv + vec2(0.1, 0.2), vec2(12.9898, 78.233))) * 43758.5453);
        float noise2 = fract(sin(dot(uv + vec2(0.3, 0.4), vec2(45.164, 94.673))) * 19726.3928);
        float noise3 = fract(sin(dot(uv + vec2(0.5, 0.6), vec2(63.928, 17.282))) * 29384.2847);
        state = vec3(noise1, noise2, noise3) * 0.5;
    }

    // Add subtle time-varying noise for organic feel
    float timeNoise = fract(sin(dot(uv, vec2(12.9898, 78.233)) + ubo.iTime * 0.01) * 43758.5453);
    state += vec3(timeNoise * 0.002);

    // Clamp to valid range
    state = clamp(state, 0.0, 1.0);

    // Color mapping for visualization
    vec3 color;

    // Create beautiful color gradient based on state
    float intensity = (state.r + state.g) * 0.5;
    float hue = state.r * 2.0 - state.g;

    // HSV to RGB-like mapping
    color = vec3(
        0.5 + 0.5 * sin(hue * 3.14159 * 2.0),
        0.5 + 0.5 * sin(hue * 3.14159 * 2.0 + 2.094),
        0.5 + 0.5 * sin(hue * 3.14159 * 2.0 + 4.189)
    ) * intensity;

    // Add glow where activity is high
    float activity = length(lap);
    color += vec3(0.1, 0.2, 0.3) * activity * 5.0;

    // Show mouse cursor
    if (ubo.iButtonLeft > 0.0 || ubo.iButtonRight > 0.0 || ubo.iButtonMiddle > 0.0) {
        float cursor = smoothstep(0.01, 0.005, distToMouse);
        color = mix(color, vec3(1.0), cursor * 0.5);
    }

    fragColor = vec4(color, 1.0);
}
