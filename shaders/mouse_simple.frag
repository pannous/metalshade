#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

/*
    Simple Mouse Input Demo
    -----------------------

    Demonstrates basic mouse interaction:
    - Move mouse: changes background color gradient
    - Click and hold: creates a bright circle at click position
    - Distance from mouse: affects brightness

    This is a minimal example showing how to use ubo.iMouse:
    - iMouse.xy = current mouse position
    - iMouse.zw = click position (negative when not pressed)
*/

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;

    // Normalize mouse coordinates to 0-1 range
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;
    vec2 clickPos = abs(ubo.iMouse.zw) / ubo.iResolution.xy;
    bool isPressed = ubo.iMouse.z > 0.0;

    // Base color controlled by mouse position
    vec3 color = vec3(mouse.x, mouse.y, 0.5);

    // Add gradient based on distance from mouse
    float distToMouse = length(uv - mouse);
    color += vec3(1.0 - distToMouse);

    // When clicked, show a bright circle at click position
    if (isPressed) {
        float distToClick = length(uv - clickPos);
        float circle = smoothstep(0.15, 0.1, distToClick);
        color += vec3(circle * 2.0);

        // Add a pulsing ring around the click
        float ring = sin(distToClick * 20.0 - ubo.iTime * 5.0) * 0.5 + 0.5;
        ring *= smoothstep(0.2, 0.15, distToClick) * smoothstep(0.1, 0.12, distToClick);
        color += vec3(ring);
    }

    fragColor = vec4(color, 1.0);
}
