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

layout(binding = 1) uniform sampler2D iChannel0;  // Static texture
layout(binding = 2) uniform sampler2D iChannel1;  // Feedback (previous frame)

/*
    Simple Feedback Test
    ---------------------

    This shader tests if feedback is working by:
    1. Reading the previous frame from iChannel1
    2. Fading it slightly
    3. Adding new content where you click

    If feedback works: Your clicks will persist and fade slowly
    If feedback doesn't work: You'll only see what you're clicking NOW
*/

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;

    // Read previous frame (THIS IS THE KEY TEST)
    vec3 prev = texture(iChannel1, uv).rgb;

    // Start with previous frame, faded slightly
    vec3 color = prev * 0.98;  // 98% persistence = slow fade

    // Add bright spot where mouse is clicked
    float dist = length(uv - mouse);

    if (ubo.iButtonLeft > 0.0) {
        // Left button: Add red
        float spot = exp(-dist * 30.0);
        color.r += spot;
    }

    if (ubo.iButtonRight > 0.0) {
        // Right button: Add green
        float spot = exp(-dist * 30.0);
        color.g += spot;
    }

    if (ubo.iButtonMiddle > 0.0) {
        // Middle button: Add blue
        float spot = exp(-dist * 30.0);
        color.b += spot;
    }

    // Show debug info: Display what we read from feedback in corner
    if (uv.x < 0.1 && uv.y > 0.9) {
        // Top-left corner: Show feedback buffer contents
        vec3 feedbackSample = texture(iChannel1, vec2(0.5, 0.5)).rgb;
        color = feedbackSample;
    }

    // If no feedback, everything will be black except current mouse
    // If feedback works, previous clicks will persist

    fragColor = vec4(color, 1.0);
}
