#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

/*
    HSB Morphing Animation
    ----------------------

    Animated transition between two HSB color representations:
    1. Rectangular gradient (hue × brightness)
    2. Color wheel (polar: angle → hue, radius → saturation)

    Features:
    - Smooth oscillating morph between representations
    - Rotating polar coordinates
    - Wave-based distortion effects
    - Radial ripples during transitions
    - Subtle brightness pulsing

    Based on The Book of Shaders Chapter 06 examples:
    - thebookofshaders/06/hsb.frag
    - thebookofshaders/06/hsb-colorwheel.frag
*/

#define TWO_PI 6.28318530718

vec3 hsb2rgb(in vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0), 6.0)-3.0)-1.0, 0.0, 1.0);
    rgb = rgb*rgb*(3.0-2.0*rgb);
    return c.z * mix(vec3(1.0), rgb, c.y);
}

void main() {
    vec2 st = fragCoord / ubo.iResolution.xy;
    vec3 color = vec3(0.0);

    // Smooth oscillation between two representations (3 second cycle)
    float morph = sin(ubo.iTime * 0.5) * 0.5 + 0.5;

    // Pulsing rotation speed increases with morph factor
    float rotation = ubo.iTime * 0.3 * morph;
    vec2 center = vec2(0.5);

    // Calculate polar coordinates with rotation
    vec2 toCenter = st - center;
    float angle = atan(toCenter.y, toCenter.x) + rotation;
    float radius = length(toCenter) * 2.0;

    // Mode 1: Rectangular HSB gradient (like hsb.frag)
    vec3 colorRect = hsb2rgb(vec3(st.x, 1.0, st.y));

    // Mode 2: Polar color wheel (like hsb-colorwheel.frag)
    vec3 colorWheel = hsb2rgb(vec3((angle/TWO_PI)+0.5, radius, 1.0));

    // Create interference wave patterns that sweep across screen
    float waveX = sin(st.x * TWO_PI * 2.0 + ubo.iTime * 2.0) * 0.5 + 0.5;
    float waveY = sin(st.y * TWO_PI * 2.0 - ubo.iTime * 2.0) * 0.5 + 0.5;
    float wavePattern = waveX * waveY;

    // Blend factor modulated by wave pattern
    float blendFactor = morph * (0.7 + wavePattern * 0.3);

    // Radial distortion effect (strongest during mid-transition)
    float transitionPeak = sin(morph * 3.14159);
    float distortion = sin(radius * 5.0 - ubo.iTime * 3.0) * 0.1 * transitionPeak;
    vec2 distortedSt = st + toCenter * distortion;
    distortedSt = clamp(distortedSt, 0.0, 1.0);

    // Recalculate rectangular mode with distortion
    vec3 colorRectDist = hsb2rgb(vec3(distortedSt.x, 1.0, distortedSt.y));

    // Smooth blend between the two representations
    color = mix(colorRectDist, colorWheel, blendFactor);

    // Subtle brightness pulse synchronized with morph
    float pulse = sin(ubo.iTime * 1.5) * 0.1 + 0.9;
    color *= pulse;

    fragColor = vec4(color, 1.0);
}
