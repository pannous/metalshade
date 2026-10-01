#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

/*
    Winamp Milkdrop Tribute
    -----------------------

    Homage to the legendary Winamp/Milkdrop visualizer era.
    Combines classic techniques from Ryan Geiss's Milkdrop:
    - Warp mesh distortion with feedback-style zoom/rotation
    - Darken-center / brighten-edge post-processing
    - Kaleidoscopic symmetry folding
    - Plasma waveforms and oscilloscope trails
    - Beat-reactive pulsing (simulated via layered sine waves)
    - IQ-style cosine palettes cycling through neon colors

    No textures required — pure math nostalgia.
*/

#define PI  3.14159265
#define TAU 6.28318530

// Cosine palette (iq's classic technique)
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {
    return a + b * cos(TAU * (c * t + d));
}

// Neon Milkdrop palette — cycles through hot pink, cyan, electric blue, green
vec3 neonPalette(float t) {
    return palette(t,
        vec3(0.5, 0.5, 0.5),
        vec3(0.5, 0.5, 0.5),
        vec3(1.0, 1.0, 1.0),
        vec3(0.0, 0.33, 0.67));
}

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

// Simulated beat — layered pulses at different "BPM" rates
float beat(float t) {
    float b1 = pow(0.5 + 0.5 * sin(t * TAU * 1.8), 8.0);   // ~108 bpm kick
    float b2 = pow(0.5 + 0.5 * sin(t * TAU * 2.4), 12.0);  // ~144 bpm snare
    float b3 = pow(0.5 + 0.5 * sin(t * TAU * 0.6), 4.0);   // slow swell
    return b1 * 0.5 + b2 * 0.3 + b3 * 0.2;
}

// 2D hash
float hash21(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

// Value noise
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i + vec2(1, 0)), f.x),
               mix(hash21(i + vec2(0, 1)), hash21(i + vec2(1, 1)), f.x), f.y);
}

// FBM — two octaves for speed
float fbm2(vec2 p) {
    return noise(p) * 0.667 + noise(p * 2.0) * 0.333;
}

// Milkdrop-style warp mesh: zoom, rotate, translate per-pixel
vec2 warpMesh(vec2 uv, float t) {
    float r = length(uv);
    float a = atan(uv.y, uv.x);

    // Radial zoom — breathes in and out
    float zoomAmt = 1.0 + 0.15 * sin(t * 0.7) + 0.08 * beat(t);
    r *= zoomAmt;

    // Rotation — accelerates near center (classic Milkdrop)
    float rotAmt = 0.1 * sin(t * 0.4) + 0.05 / (r + 0.3);
    a += rotAmt;

    // Reconstruct
    uv = vec2(cos(a), sin(a)) * r;

    // Slight wave displacement
    uv.x += 0.03 * sin(uv.y * 8.0 + t * 2.0);
    uv.y += 0.03 * cos(uv.x * 8.0 - t * 1.5);

    return uv;
}

// Oscilloscope ring — simulates waveform wrapped around a circle
float scopeRing(vec2 uv, float t, float radius, float freq, float amp) {
    float a = atan(uv.y, uv.x);
    float r = length(uv);

    // Waveform displacement
    float wave = sin(a * freq + t * 4.0) * amp;
    wave += sin(a * freq * 0.5 - t * 3.0) * amp * 0.5;
    wave *= 0.5 + 0.5 * beat(t);

    float ring = abs(r - radius - wave);
    return smoothstep(0.015, 0.0, ring);
}

// Plasma field — classic Winamp plasma
float plasma(vec2 p, float t) {
    float v = sin(p.x * 3.0 + t);
    v += sin(p.y * 3.0 + t * 1.3);
    v += sin((p.x + p.y) * 2.0 + t * 0.7);
    v += sin(length(p) * 4.0 - t * 2.0);
    return v * 0.25;
}

// Kaleidoscopic fold — Milkdrop symmetry
vec2 kaleidoscope(vec2 uv, float segments) {
    float a = atan(uv.y, uv.x);
    float r = length(uv);
    float segAngle = TAU / segments;
    a = mod(a, segAngle);
    a = abs(a - segAngle * 0.5);
    return vec2(cos(a), sin(a)) * r;
}

void main() {
    vec2 uv = (fragCoord - 0.5 * ubo.iResolution.xy) / ubo.iResolution.y;
    float t = ubo.iTime;
    float bt = beat(t);

    // Scene selection — cycles through visual presets like Milkdrop
    float scene = mod(t * 0.08, 1.0);  // slow cycle through looks

    // --- Layer 1: Warp mesh background ---
    vec2 wuv = warpMesh(uv, t);

    // Kaleidoscope with time-varying segment count
    float segments = 4.0 + 4.0 * floor(sin(t * 0.13) * 2.0 + 2.5);
    vec2 kuv = kaleidoscope(wuv, segments);

    // Plasma background through warped + kaleidoscoped coords
    float p1 = plasma(kuv * 3.0, t);
    float p2 = plasma(kuv * 2.0 + vec2(t * 0.3), t * 0.8);
    float plasmaVal = p1 * 0.6 + p2 * 0.4;

    // Color the plasma with the neon palette
    float colorIndex = plasmaVal + t * 0.15 + length(kuv) * 0.5;
    vec3 col = neonPalette(colorIndex) * (0.4 + 0.3 * plasmaVal);

    // --- Layer 2: Tunnel zoom lines ---
    float tunnelAngle = atan(uv.y, uv.x);
    float tunnelR = length(uv);
    float tunnelLines = sin(tunnelAngle * 12.0 + t * 3.0);
    tunnelLines *= sin(1.0 / (tunnelR + 0.1) * 2.0 - t * 5.0);
    float tunnelMask = smoothstep(0.3, 0.0, tunnelR) * 0.4;
    col += neonPalette(tunnelAngle / TAU + t * 0.2) * max(tunnelLines, 0.0) * tunnelMask;

    // --- Layer 3: Oscilloscope waveform rings ---
    float scope1 = scopeRing(uv, t, 0.3 + 0.05 * sin(t * 0.5), 7.0, 0.04);
    float scope2 = scopeRing(uv, t * 1.1, 0.5 + 0.08 * cos(t * 0.4), 5.0, 0.03);
    float scope3 = scopeRing(uv, t * 0.9, 0.15 + 0.03 * sin(t * 0.7), 11.0, 0.02);

    col += neonPalette(t * 0.3) * scope1 * 1.5;
    col += neonPalette(t * 0.3 + 0.33) * scope2 * 1.2;
    col += neonPalette(t * 0.3 + 0.67) * scope3 * 1.0;

    // --- Layer 4: Beat flash ---
    col += vec3(1.0) * bt * 0.15;

    // --- Layer 5: Center glow (darken center / brighten edge, classic Milkdrop) ---
    float r = length(uv);
    float centerDarken = smoothstep(0.0, 0.4, r);
    float edgeBrighten = smoothstep(0.5, 0.9, r);
    col *= 0.6 + 0.4 * centerDarken;
    col += neonPalette(t * 0.1 + r) * edgeBrighten * 0.15;

    // --- Layer 6: Starfield sparkle ---
    vec2 starUV = uv * 30.0;
    float sparkle = hash21(floor(starUV));
    sparkle = pow(sparkle, 40.0);
    sparkle *= sin(t * 10.0 + sparkle * 100.0) * 0.5 + 0.5;
    col += vec3(sparkle) * 0.6;

    // --- Post-processing ---
    // Milkdrop-style gamma brightening
    col = pow(max(col, 0.0), vec3(0.75));

    // Subtle noise grain (CRT feel)
    float grain = hash21(uv * 500.0 + fract(t * 60.0)) * 0.04;
    col += grain;

    // Vignette
    float vignette = 1.0 - dot(uv, uv) * 0.4;
    col *= vignette;

    // Soft clamp
    col = clamp(col, 0.0, 1.0);

    fragColor = vec4(col, 1.0);
}
