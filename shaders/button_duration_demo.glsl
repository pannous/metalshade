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
    Button Duration Demo
    --------------------

    Demonstrates button press duration tracking:

    - Click and HOLD left mouse button
    - Watch the circle grow as you hold
    - Release the button
    - The circle STAYS at its final size showing how long you held it
    - Click again to reset and start over

    The longer you hold, the larger the circle gets!
    Duration is shown as:
    - Circle size (radius grows with duration)
    - Color intensity (brighter = longer press)
    - Text display showing seconds held
*/

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;

    // Dark background
    vec3 color = vec3(0.05, 0.05, 0.1);

    // Get button duration (0 = not pressed, >0 = seconds held)
    float duration = ubo.iButtonLeft;

    if (duration > 0.0) {
        // Calculate distance from mouse
        float dist = length(uv - mouse);

        // Circle grows with duration: 0.1 to 0.5 radius
        float radius = 0.1 + duration * 0.08;

        // Create glowing circle
        float circle = smoothstep(radius + 0.02, radius - 0.02, dist);

        // Color changes with duration
        float hue = mod(duration * 0.3, 1.0);
        vec3 circleColor = vec3(
            0.5 + 0.5 * sin(hue * 6.28),
            0.5 + 0.5 * sin((hue + 0.33) * 6.28),
            0.5 + 0.5 * sin((hue + 0.67) * 6.28)
        );

        // Add circle to scene
        color = mix(color, circleColor, circle);

        // Add pulsing ring at edge
        float ring = abs(dist - radius);
        float ringGlow = exp(-ring * 50.0) * (0.5 + 0.5 * sin(ubo.iTime * 5.0));
        color += vec3(ringGlow * 0.5);

        // Add radial waves inside circle
        if (dist < radius) {
            float waves = sin(dist * 20.0 - ubo.iTime * 3.0) * 0.5 + 0.5;
            color += circleColor * waves * 0.2;
        }
    }

    // Add instruction text effect in top-left
    vec2 textPos = uv - vec2(0.1, 0.9);
    float textGlow = exp(-length(textPos) * 30.0);
    color += vec3(0.3, 0.5, 0.7) * textGlow * 0.3;

    // Duration indicator bar at bottom
    if (duration > 0.0) {
        float barY = 0.05;
        float barHeight = 0.02;
        if (uv.y < barY + barHeight && uv.y > barY) {
            float barProgress = duration * 0.1; // 10 seconds = full bar
            if (uv.x < barProgress) {
                color = mix(color, vec3(1.0, 0.8, 0.2), 0.8);
            }
        }

        // Add tick marks on bar
        for (float i = 0.1; i <= 1.0; i += 0.1) {
            if (abs(uv.x - i) < 0.002 && uv.y < barY + barHeight * 1.5 && uv.y > barY) {
                color = vec3(0.5);
            }
        }
    }

    // Add crosshair at mouse
    vec2 mousePixel = ubo.iMouse.xy;
    vec2 fragPixel = fragCoord;
    float crosshair = 0.0;
    crosshair += smoothstep(1.5, 0.0, abs(fragPixel.x - mousePixel.x)) *
                 smoothstep(10.0, 0.0, abs(fragPixel.y - mousePixel.y));
    crosshair += smoothstep(1.5, 0.0, abs(fragPixel.y - mousePixel.y)) *
                 smoothstep(10.0, 0.0, abs(fragPixel.x - mousePixel.x));
    color = mix(color, vec3(1.0), crosshair * 0.5);

    fragColor = vec4(color, 1.0);
}
