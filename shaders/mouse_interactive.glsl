#version 450
//  uniform buffer alignment issues YIKES
layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
    vec2 iScroll;  // Accumulated scroll offset (x, y)
    // Button durations (0.0 = not pressed, >0.0 = duration in seconds)
    float iButtonLeft;
    float iButtonRight;
    float iButtonMiddle;
    float iButton4;
    float iButton5;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;

/*
    Interactive Mouse Shader
    ------------------------

    Features:
    - Click and drag to create ripple effects
    - Mouse position controls color hue
    - Distance from mouse affects saturation
    - Multiple overlapping ripples from different clicks
    - Particle trails that follow mouse movement

    Controls:
    - Move mouse to change colors
    - Left click: Creates ripples + red pulsing tint
    - Right click: Bright green corners
    - Middle click: Blue vertical stripes
    - Button 4 (back): Magenta horizontal wave
    - Button 5 (forward): Cyan radial pulse from center
    - Scroll wheel: Zoom in/out (centered on mouse)
    - Press R: Reset scroll/zoom
*/

#define PI 3.14159265359

// HSB to RGB conversion
vec3 hsb2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0), 6.0)-3.0)-1.0, 0.0, 1.0);
    rgb = rgb*rgb*(3.0-2.0*rgb);
    return c.z * mix(vec3(1.0), rgb, c.y);
}

// Create smooth ripple wave
float ripple(vec2 uv, vec2 center, float time, float speed) {
    float dist = length(uv - center);
    float wave = sin(dist * 15.0 - time * speed) * 0.5 + 0.5;
    float decay = exp(-dist * 2.0);
    return wave * decay;
}

void main() {
    vec2 uv = fragCoord / ubo.iResolution.xy;
    vec2 mouse = ubo.iMouse.xy / ubo.iResolution.xy;
    vec2 clickPos = abs(ubo.iMouse.zw) / ubo.iResolution.xy;
    bool isPressed = ubo.iMouse.z > 0.0;

    // Calculate zoom from scroll (scroll up = zoom in)
    float zoom = exp(ubo.iScroll.y * 0.1);  // Exponential zoom feels natural
    zoom = clamp(zoom, 0.1, 10.0);  // Limit zoom range

    // Apply zoom centered on mouse position
    vec2 zoomedUv = mouse + (uv - mouse) / zoom;

    // Calculate distance from mouse (use zoomed coordinates for effects)
    float distToMouse = length(zoomedUv - mouse);

    // Base color influenced by mouse position
    float hue = mouse.x;
    float brightness = 0.3 + mouse.y * 0.5;

    // Create ripples from click position when mouse is pressed
    float rippleEffect = 0.0;
    if (isPressed) {
        rippleEffect = ripple(zoomedUv, clickPos, ubo.iTime, 5.0);
        // Add additional ripples at different frequencies
        rippleEffect += ripple(zoomedUv, clickPos, ubo.iTime * 1.3, 7.0) * 0.5;
        rippleEffect += ripple(zoomedUv, clickPos, ubo.iTime * 0.7, 3.0) * 0.3;
    }

    // Create a glow effect around the mouse cursor
    float cursorGlow = exp(-distToMouse * 8.0);

    // Swirling effect based on angle to mouse
    vec2 toMouse = mouse - zoomedUv;
    float angle = atan(toMouse.y, toMouse.x);
    float spiralHue = hue + angle / (2.0 * PI) + distToMouse * 2.0;

    // Combine effects
    float saturation = 0.6 + cursorGlow * 0.4 + rippleEffect * 0.3;
    saturation = clamp(saturation, 0.0, 1.0);

    brightness += rippleEffect * 0.4 + cursorGlow * 0.3;
    brightness = clamp(brightness, 0.0, 1.0);

    // Create color with HSB
    vec3 color = hsb2rgb(vec3(spiralHue, saturation, brightness));

    // Add mouse trail effect - brighter along the path to mouse
    float trail = 0.0;
    for (float i = 0.0; i < 5.0; i++) {
        float t = i / 5.0;
        vec2 trailPos = mix(clickPos, mouse, t);
        float trailDist = length(zoomedUv - trailPos);
        trail += exp(-trailDist * 30.0) * (1.0 - t * 0.5);
    }
    color += vec3(trail * 0.5);

    // Add circular waves emanating from mouse when clicked
    if (isPressed) {
        float wave = sin(distToMouse * 30.0 - ubo.iTime * 8.0) * 0.5 + 0.5;
        float waveMask = smoothstep(0.4, 0.6, wave) * exp(-distToMouse * 3.0);
        color += vec3(waveMask * 0.4);
    }

    // Subtle animated background texture (use zoomed coordinates)
    vec2 texCoord = zoomedUv + vec2(sin(zoomedUv.y * 10.0 + ubo.iTime * 0.5) * 0.02,
                                     cos(zoomedUv.x * 10.0 + ubo.iTime * 0.5) * 0.02);
    vec3 texColor = texture(iChannel0, texCoord).rgb;
    color = mix(color, texColor * color, 0.2);

    // OBVIOUS BUTTON EFFECTS - each button gets a distinct color and position
    // Left button (already creates ripples) - MASSIVE red screen flash
    if (ubo.iButtonLeft > 0.0) {
        color = mix(color, vec3(1.0, 0.0, 0.0), 0.5 * (sin(ubo.iButtonLeft * 10.0) * 0.5 + 0.5));
    }

    // Right button - BRIGHT green full screen overlay
    if (ubo.iButtonRight > 0.0) {
        color = mix(color, vec3(0.0, 1.0, 0.0), 0.6);
    }

    // Middle button - BRIGHT blue full screen with animated stripes
    if (ubo.iButtonMiddle > 0.0) {
        float stripes = sin(uv.x * 50.0 + ubo.iButtonMiddle * 5.0) * 0.5 + 0.5;
        color = mix(color, vec3(0.0, 0.0, 1.0), 0.5 + stripes * 0.3);
    }

    // Button 4 - MASSIVE magenta screen takeover
    if (ubo.iButton4 > 0.0) {
        float wave = sin(uv.y * 30.0 - ubo.iButton4 * 8.0) * 0.5 + 0.5;
        color = mix(color, vec3(1.0, 0.0, 1.0), 0.7 * wave);
    }

    // Button 5 - BRIGHT cyan full screen pulse
    if (ubo.iButton5 > 0.0) {
        float centerDist = length(uv - vec2(0.5));
        float pulse = sin(centerDist * 20.0 - ubo.iButton5 * 10.0) * 0.5 + 0.5;
        color = mix(color, vec3(0.0, 1.0, 1.0), 0.8 * pulse);
    }

    // Add a bright crosshair at exact mouse position for debugging/visibility
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
    color = mix(color, vec3(1.0, 1.0, 0.0), crosshair * 0.7); // Yellow crosshair

    fragColor = vec4(color, 1.0);
}
