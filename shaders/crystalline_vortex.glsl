#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

#define MAX_STEPS 100
#define MAX_DIST 100.0
#define SURF_DIST 0.001

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

// Smooth minimum for blending shapes
float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// Hash function for procedural texture
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3D noise function
float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);

    return mix(mix(mix(hash(p + vec3(0,0,0)), hash(p + vec3(1,0,0)), f.x),
                   mix(hash(p + vec3(0,1,0)), hash(p + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(p + vec3(0,0,1)), hash(p + vec3(1,0,1)), f.x),
                   mix(hash(p + vec3(0,1,1)), hash(p + vec3(1,1,1)), f.x), f.y), f.z);
}

// Fractal brownian motion
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for(int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Scene distance function
float getDist(vec3 p) {
    vec3 q = p;
    float t = ubo.iTime;

    // Gentle rotation with varying speeds
    q.xz *= rot(t * 0.15 + sin(t * 0.3) * 0.3);
    q.xy *= rot(t * 0.1 + cos(t * 0.25) * 0.2);

    // Subtle twisting - keep structures recognizable
    float twistStrength = 0.15 + sin(t * 0.5) * 0.08;
    float twist = length(q.xz) * 1.2 - t * 0.4;
    q.xz *= rot(twist * twistStrength);

    // Modulo repetition for infinite tunnel
    vec3 id = floor(q / 4.0);
    q = mod(q + 2.0, 4.0) - 2.0;

    // Animated noise displacement
    float n = fbm(q * 0.8 + vec3(t * 0.3, t * 0.2, t * 0.25));

    // Pulsing crystal size
    float pulse = 0.8 + sin(t * 1.5 + length(q) * 2.0) * 0.3;

    // Crystal formation with morphing shape
    float morphAmount = sin(t * 0.8) * 0.5 + 0.5;
    vec3 qMorph = mix(abs(q), q * q * sign(q), morphAmount);
    float crystal = length(qMorph - pulse) - 0.15;
    crystal += n * (0.4 + sin(t) * 0.2);

    // Create spiky protrusions with growth animation
    float spikeGrowth = 0.5 + sin(t * 1.2) * 0.3;
    float spikes = length(q) - spikeGrowth;

    for(int i = 0; i < 6; i++) {
        vec3 r = q;
        float angle = float(i) * 3.14159 / 3.0 + t * 0.8;
        r.xz *= rot(angle);
        r.xy *= rot(sin(t * 0.6 + float(i)) * 0.5);

        // Breathing spike length
        float spikeLen = 0.7 + sin(t * 1.5 + float(i) * 0.5) * 0.2;
        float spikeWidth = 0.1 + sin(t * 2.0 + float(i)) * 0.05;
        float spike = length(r - vec3(spikeLen, 0, 0)) - spikeWidth;
        spikes = smin(spikes, spike, 0.2);
    }

    // Blend shapes with animated smoothness
    float blendAmount = 0.3 + sin(t * 0.9) * 0.2;
    float d = smin(crystal, spikes, blendAmount);

    // Pulsing tunnel walls
    float tunnelSize = 5.0 + sin(t * 0.6) * 0.5;
    float tunnel = -(length(p.xz) - tunnelSize);
    d = smin(d, tunnel, 0.8);

    return d * 0.8;
}

// Calculate normal
vec3 getNormal(vec3 p) {
    vec2 e = vec2(0.001, 0);
    return normalize(vec3(
        getDist(p + e.xyy) - getDist(p - e.xyy),
        getDist(p + e.yxy) - getDist(p - e.yxy),
        getDist(p + e.yyx) - getDist(p - e.yyx)
    ));
}

// Ray marching
float rayMarch(vec3 ro, vec3 rd) {
    float d = 0.0;
    for(int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * d;
        float ds = getDist(p);
        d += ds;
        if(ds < SURF_DIST || d > MAX_DIST) break;
    }
    return d;
}

// Color palette with time-varying parameters
vec3 palette(float t) {
    // Animate the palette parameters themselves
    float phase = ubo.iTime * 0.1;
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263 + sin(phase) * 0.2, 0.416 + cos(phase * 1.3) * 0.2, 0.557 + sin(phase * 0.8) * 0.2);
    return a + b * cos(6.28318 * (c * t + d));
}

void main() {
    vec2 uv = (fragCoord - 0.5 * ubo.iResolution.xy) / ubo.iResolution.y;
    float t = ubo.iTime / 3.0;

    // Camera setup - slow gentle movement, staying in the sweet spot
    float camSpeed = 0.3 + sin(t * 0.2) * 0.15;
    vec3 ro = vec3(sin(t * 0.3) * 1.2, cos(t * 0.25) * 0.8, -5 + t * camSpeed);
    vec3 rd = normalize(vec3(uv.x, uv.y, 1.0));

    // Dynamic camera rotation - explore the structures
    rd.xz *= rot(sin(t * 0.4) * 0.8 + t * 0.05);
    rd.xy *= rot(sin(t * 0.35) * 0.5 + cos(t * 0.3) * 0.3);

    // Ray march
    float d = rayMarch(ro, rd);
    vec3 col = vec3(0);

    if(d < MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = getNormal(p);

        // Dynamic lighting - keep it near the structures
        vec3 lightPos = vec3(sin(t * 0.6) * 2.5, 2.0 + cos(t * 0.8) * 1.0, -3.0 + t * camSpeed);
        vec3 l = normalize(lightPos - p);
        float dif = clamp(dot(n, l), 0.0, 1.0);

        // Ambient occlusion
        float ao = 1.0 - float(MAX_STEPS) / 100.0;

        // Animated fresnel effect
        float fresnelPower = 3.0 + sin(t * 0.8) * 1.0;
        float fresnel = pow(1.0 - max(dot(n, -rd), 0.0), fresnelPower);

        // Color based on position and time with multiple frequencies
        float colorT = length(p.xz) * 0.1 + p.y * 0.2 + t * 0.5;
        colorT += sin(length(p) * 2.0 - t * 2.0) * 0.2;
        vec3 baseCol = palette(colorT);

        // Animated procedural texture
        float tex = fbm(p * 2.0 + vec3(t * 0.1, t * 0.15, t * 0.12));
        baseCol *= 0.8 + tex * 0.4;

        // Pulsing ambient light - reduced for contrast
        float ambientPulse = 0.1 + sin(t * 1.5) * 0.05;

        // Combine lighting with higher contrast
        col = baseCol * dif * dif * ao;
        col += fresnel * palette(colorT + 0.5 + sin(t * 0.7) * 0.3) * (0.6 + sin(t) * 0.2);
        col += baseCol * ambientPulse;

        // Animated glow with color shift
        float glow = 1.0 / (1.0 + d * d * 0.1);
        col += palette(colorT + t * 0.7) * glow * (0.3 + sin(t * 1.2) * 0.15);
    }

    // Animated fog with color cycling
    vec3 fogColor = palette(t * 0.15 + length(uv) * 0.3) * (0.1 + sin(t * 0.5) * 0.05);
    float fogAmount = 1.0 - exp(-0.0005 * d * d);
    col = mix(col, fogColor, fogAmount);

    // Pulsing vignette
    float vignette = 1.0 - dot(uv, uv) * (0.3 + sin(t * 0.6) * 0.1);
    col *= vignette;

    // Add chromatic shift at edges
    float chromatic = length(uv) * (0.3 + sin(t) * 0.1);
    col += palette(t * 0.3 + chromatic) * chromatic * 0.1;

    // Tone mapping with increased contrast
    col = col / (col + vec3(0.8));
    col = pow(col, vec3(0.4)); // Gamma correction with more contrast

    fragColor = vec4(col, 1.0);
}
