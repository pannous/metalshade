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

layout(binding = 1) uniform sampler2D iChannel0;  // Static texture (not used)
layout(binding = 2) uniform sampler2D iChannel1;  // Previous frame (feedback)

/*
    Paint Program Shader
    --------------------

    A proper paint program that remembers your brush strokes!

    Controls:
    - Left Click + Drag: Draw with cyan brush
    - Right Click + Drag: Draw with magenta brush
    - Middle Click + Drag: Draw with yellow brush
    - Button 4: Erase (fade to black)
    - Button 5: Clear canvas completely

    How it works:
    - Reads the previous frame from iChannel1
    - Adds new brush strokes on top
    - Fades very slowly over time for a persistent effect
*/

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;

    // Read previous frame - this is the KEY to persistence!
    vec4 prevColor = texture(iChannel1, uv);

    // Very slow fade for persistence (0.995 means 99.5% stays)
    // This gives a subtle fade over many frames
    vec3 color = prevColor.rgb * 0.995;

    // Button states
    float leftPressed = ubo.iButtonLeft;
    float rightPressed = ubo.iButtonRight;
    float middlePressed = ubo.iButtonMiddle;
    float button4Pressed = ubo.iButton4;
    float button5Pressed = ubo.iButton5;

    // Draw brush strokes when buttons are pressed
    float distToMouse = length((fragCoord / ubo.iResolution.xy) - mouse);

    // Left button: Cyan brush
    if (leftPressed > 0.0) {
        float brushSize = 0.02;  // Brush radius
        float brushStrength = smoothstep(brushSize, 0.0, distToMouse);
        vec3 brushColor = vec3(0.0, 1.0, 1.0);  // Cyan
        color = mix(color, brushColor, brushStrength * 0.8);
    }

    // Right button: Magenta brush
    if (rightPressed > 0.0) {
        float brushSize = 0.03;  // Slightly larger brush
        float brushStrength = smoothstep(brushSize, 0.0, distToMouse);
        vec3 brushColor = vec3(1.0, 0.0, 0.8);  // Magenta
        color = mix(color, brushColor, brushStrength * 0.8);
    }

    // Middle button: Yellow/gold brush
    if (middlePressed > 0.0) {
        float brushSize = 0.015;  // Smaller brush
        float brushStrength = smoothstep(brushSize, 0.0, distToMouse);
        vec3 brushColor = vec3(1.0, 0.9, 0.0);  // Yellow
        color = mix(color, brushColor, brushStrength * 0.9);
    }

    // Button 4: Eraser (fast fade)
    if (button4Pressed > 0.0) {
        color *= 0.9;  // Faster fade when erasing
    }

    // Button 5: Clear canvas
    if (button5Pressed > 0.0) {
        color = vec3(0.0);  // Black
    }

    // Add a subtle cursor indicator
    if (leftPressed > 0.0 || rightPressed > 0.0 || middlePressed > 0.0) {
        float cursor = smoothstep(0.005, 0.003, distToMouse);
        color = mix(color, vec3(1.0), cursor * 0.3);
    }

    fragColor = vec4(color, 1.0);
}
