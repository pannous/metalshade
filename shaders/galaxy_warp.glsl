#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

// Hash for stars
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float hash13(vec3 p3) {
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

// Smooth noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal brownian motion for nebula
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for(int i = 0; i < 6; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Generate procedural galaxy
vec3 galaxy(vec2 p, float time) {
    // Convert to polar coordinates
    float angle = atan(p.y, p.x);
    float radius = length(p);

    // Spiral arms
    float spiral = sin(angle * 2.0 - radius * 3.0 + time * 0.5) * 0.5 + 0.5;
    spiral += sin(angle * 2.0 - radius * 3.0 + 3.14159 + time * 0.5) * 0.5 + 0.5;

    // Radial falloff for galaxy shape
    float galaxyShape = exp(-radius * 1.5);

    // Add nebula clouds using FBM
    vec2 nebCoord = p * 2.0;
    nebCoord *= rot(time * 0.1);
    float nebula = fbm(nebCoord + time * 0.1);
    nebula = pow(nebula, 2.0);

    // Combine spiral arms with nebula
    float density = spiral * galaxyShape * 0.8;
    density += nebula * galaxyShape * 0.6;

    // Core glow
    float core = 1.0 / (1.0 + radius * radius * 8.0);
    density += core * 2.0;

    // Color based on density and radius
    vec3 col = vec3(0.0);

    // Blue-purple outer regions
    col += vec3(0.3, 0.4, 1.0) * density * (1.0 - radius * 0.5);

    // Orange-yellow core
    col += vec3(1.0, 0.6, 0.2) * core * 2.0;

    // Pink nebula
    col += vec3(1.0, 0.3, 0.5) * nebula * galaxyShape * 0.8;

    // Add stars
    vec2 starCoord = p * 40.0;
    starCoord *= rot(time * 0.05);
    float stars = hash(floor(starCoord));
    stars = pow(stars, 50.0) * 5.0;
    col += vec3(stars);

    return col;
}

// Gravitational lensing / spacetime warp
vec2 gravitationalLens(vec2 uv, vec2 massPos, float mass, float time) {
    vec2 toMass = uv - massPos;
    float dist = length(toMass);

    // Einstein ring radius
    float schwarzschild = mass * 0.3;

    // Lensing effect - bend light around massive object
    float lensStrength = schwarzschild / (dist + 0.1);

    // Rotate around the mass
    float rotation = lensStrength * 2.0;
    toMass *= rot(rotation);

    // Radial distortion
    float distortion = 1.0 + lensStrength * 0.5;
    vec2 warpedUV = massPos + toMass * distortion;

    return warpedUV;
}

void main() {
    vec2 uv = (fragCoord - 0.5 * ubo.iResolution.xy) / ubo.iResolution.y;
    float t = ubo.iTime * 0.3;

    // Rotate view
    uv *= rot(t * 0.1);

    // Create multiple gravitational lenses (black holes / massive objects)
    vec2 mass1 = vec2(sin(t * 0.5) * 0.4, cos(t * 0.5) * 0.3);
    vec2 mass2 = vec2(cos(t * 0.4) * 0.5, sin(t * 0.3) * 0.4);

    // Apply gravitational lensing
    vec2 warpedUV = uv;
    warpedUV = gravitationalLens(warpedUV, mass1, 0.4 + sin(t) * 0.1, t);
    warpedUV = gravitationalLens(warpedUV, mass2, 0.3 + cos(t * 1.3) * 0.1, t);

    // Add time-varying distortion waves
    float wave = sin(length(uv) * 5.0 - t * 2.0) * 0.05;
    warpedUV += normalize(uv) * wave;

    // Sample galaxy at warped coordinates
    vec3 col = galaxy(warpedUV, t);

    // Add lens flare around masses
    float flare1 = 0.02 / (length(uv - mass1) + 0.02);
    float flare2 = 0.02 / (length(uv - mass2) + 0.02);
    col += vec3(0.5, 0.7, 1.0) * flare1 * 0.3;
    col += vec3(1.0, 0.5, 0.3) * flare2 * 0.3;

    // Add accretion disk glow around masses
    float dist1 = length(uv - mass1);
    float disk1 = smoothstep(0.15, 0.05, abs(dist1 - 0.1)) * 2.0;
    col += vec3(1.0, 0.4, 0.1) * disk1;

    float dist2 = length(uv - mass2);
    float disk2 = smoothstep(0.12, 0.05, abs(dist2 - 0.08)) * 2.0;
    col += vec3(0.3, 0.5, 1.0) * disk2;

    // Vignette
    float vignette = 1.0 - length(uv) * 0.5;
    vignette = smoothstep(0.3, 1.0, vignette);
    col *= vignette;

    // Tone mapping
    col = col / (col + vec3(0.8));
    col = pow(col, vec3(0.85));

    fragColor = vec4(col, 1.0);
}
