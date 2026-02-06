#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
    vec2 iScroll;  // Accumulated scroll offset (x, y)
} ubo;

/*
    Simple Mouse Input Demo
    -----------------------

    Demonstrates basic mouse interaction:
    - Move mouse: changes background color gradient
    - Click and hold: creates a bright circle at click position
    - Distance from mouse: affects brightness

    This is a minimal example showing how to use mouse and scroll input:
    - iMouse.xy = current mouse position
    - iMouse.zw = click position (negative when not pressed)
    - iScroll.xy = accumulated scroll offset
    - Use scroll for zoom, intensity, or any shader-specific parameter
*/

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;

    // Normalize mouse coordinates to 0-1 range
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;
    vec2 clickPos = abs(ubo.iMouse.zw) / ubo.iResolution.xy;
    bool isPressed = ubo.iMouse.z > 0.0;

    // Calculate zoom from scroll (scroll up = zoom in)
    float zoom = exp(ubo.iScroll.y * 0.1);
    zoom = clamp(zoom, 0.1, 10.0);

    // Apply zoom centered on mouse position
    vec2 zoomedUv = mouse + (uv - mouse) / zoom;

    // Base color controlled by mouse position
    vec3 color = vec3(mouse.x, mouse.y, 0.5);

    // Add gradient based on distance from mouse (use zoomed coordinates)
    float distToMouse = length(zoomedUv - mouse);
    color += vec3(1.0 - distToMouse);

    // When clicked, show a bright circle at click position (use zoomed coordinates)
    if (isPressed) {
        float distToClick = length(zoomedUv - clickPos);
        float circle = smoothstep(0.15, 0.1, distToClick);
        color += vec3(circle * 2.0);

        // Add a pulsing ring around the click
        float ring = sin(distToClick * 20.0 - ubo.iTime * 5.0) * 0.5 + 0.5;
        ring *= smoothstep(0.2, 0.15, distToClick) * smoothstep(0.1, 0.12, distToClick);
        color += vec3(ring);
    }

    // Add a bright crosshair at exact mouse position for verification
    vec2 mousePixel = ubo.iMouse.xy;
    vec2 fragPixel = fragCoord;

    // Vertical line
    float crosshairV = smoothstep(2.0, 0.0, abs(fragPixel.x - mousePixel.x)) *
                       smoothstep(15.0, 0.0, abs(fragPixel.y - mousePixel.y));
    // Horizontal line
    float crosshairH = smoothstep(2.0, 0.0, abs(fragPixel.y - mousePixel.y)) *
                       smoothstep(15.0, 0.0, abs(fragPixel.x - mousePixel.x));
    // Center dot
    float centerDot = smoothstep(3.0, 1.0, length(fragPixel - mousePixel));

    float crosshair = max(max(crosshairV, crosshairH), centerDot);
    color = mix(color, vec3(1.0, 1.0, 0.0), crosshair * 0.8); // Yellow crosshair

    fragColor = vec4(color, 1.0);
}
