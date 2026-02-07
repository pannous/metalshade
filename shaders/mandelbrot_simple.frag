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
layout(binding = 2) uniform sampler2D iChannel1;

/*
    Simple Mandelbrot Set
    ----------------------

    The Mandelbrot set is the set of complex numbers c for which the function
    f(z) = z² + c does not diverge when iterated from z = 0.

    Algorithm:
    1. For each pixel, convert to complex number c
    2. Start with z = 0
    3. Iterate: z = z² + c
    4. If |z| > 2, the point escapes (not in set)
    5. Count how many iterations before escape
    6. Color based on iteration count

    Controls:
    - Scroll: Zoom in/out
    - Mouse: Center view on mouse position
    - Left click: Reset zoom

    No optimizations - pure clarity!
*/

// Maximum iterations before we give up
// Higher = more detail but slower
const int MAX_ITERATIONS = 100;

// Escape radius - if |z| > 2, it will escape to infinity
const float ESCAPE_RADIUS = 2.0;

void main() {
    // Convert pixel coordinates to normalized (0 to 1)
    vec2 uv = fragCoord / ubo.iResolution.xy;

    // Mouse position (normalized)
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;

    // Zoom level (scroll controls zoom)
    // Negative scroll = zoom out, positive = zoom in
    float zoom = exp(-ubo.iScroll.y * 0.1);
    zoom = clamp(zoom, 0.01, 100.0);

    // Reset zoom on left click
    if (ubo.iButtonLeft > 0.0) {
        zoom = 1.0;
    }

    // Center position (follows mouse, or stays at origin)
    vec2 center = vec2(-0.5, 0.0);  // Default: interesting part of set
    if (ubo.iMouse.z > 0.0) {  // If mouse clicked at least once
        // Convert mouse to complex plane coordinates
        center = (mouse - 0.5) * 3.0 / zoom;
    }

    // Convert screen coordinates to complex plane
    // Map from [0,1] to complex plane around center
    vec2 c;
    c.x = (uv.x - 0.5) * 3.0 / zoom + center.x;  // Real part
    c.y = (uv.y - 0.5) * 3.0 / zoom + center.y;  // Imaginary part

    // Adjust aspect ratio so circles are circles
    c.x *= ubo.iResolution.x / ubo.iResolution.y;

    // === MANDELBROT ITERATION ===

    // Start with z = 0 + 0i
    vec2 z = vec2(0.0, 0.0);

    // Iteration counter
    int iterations = 0;

    // Keep iterating while z hasn't escaped
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        // Complex multiplication: z² = (a + bi)² = (a² - b²) + (2ab)i
        float z_real = z.x * z.x - z.y * z.y;
        float z_imag = 2.0 * z.x * z.y;

        // z = z² + c
        z.x = z_real + c.x;
        z.y = z_imag + c.y;

        // Check if z has escaped (|z| > 2 means it will go to infinity)
        float magnitude = length(z);
        if (magnitude > ESCAPE_RADIUS) {
            break;  // Escaped! This point is not in the set
        }

        iterations++;
    }

    // === COLORING ===

    vec3 color;

    if (iterations == MAX_ITERATIONS) {
        // Point is in the Mandelbrot set - color it black
        color = vec3(0.0, 0.0, 0.0);
    } else {
        // Point escaped - color based on how quickly

        // Normalize iteration count to 0-1 range
        float t = float(iterations) / float(MAX_ITERATIONS);

        // Simple smooth coloring
        // Method 1: Rainbow gradient
        color = 0.5 + 0.5 * cos(3.0 + t * 12.0 + vec3(0.0, 0.5, 1.0));

        // Brighten colors for visibility
        color = pow(color, vec3(0.8));
    }

    // Add subtle animation to show it's working
    float pulse = sin(ubo.iTime * 2.0) * 0.05 + 0.95;
    color *= pulse;

    fragColor = vec4(color, 1.0);
}
