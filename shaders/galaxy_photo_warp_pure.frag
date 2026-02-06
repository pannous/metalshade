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

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for(int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Generate realistic NGC 4414-style spiral galaxy
vec3 galaxyPhoto(vec2 p) {
    float angle = atan(p.y, p.x);
    float radius = length(p);

    // Rotate galaxy slightly
    p *= rot(0.3);
    angle = atan(p.y, p.x);

    // Spiral arms - NGC 4414 has 2 prominent arms
    float armAngle = angle * 1.5 - radius * 4.0;
    float arm1 = smoothstep(0.3, 0.0, abs(sin(armAngle)));
    float arm2 = smoothstep(0.3, 0.0, abs(sin(armAngle + 3.14159)));

    float spiralArms = max(arm1, arm2);

    // Add dust lanes (dark bands in spiral arms)
    float dust = fbm(p * 15.0);
    float dustLanes = smoothstep(0.5, 0.7, dust);
    spiralArms *= mix(0.3, 1.0, dustLanes);

    // Core - bright yellow-white center
    float core = exp(-radius * radius * 8.0);
    float coreGlow = exp(-radius * radius * 2.0);

    // Disk - overall galaxy brightness falloff
    float disk = exp(-radius * 1.5);

    // Star forming regions (blue knots in arms)
    vec2 starRegions = p * 12.0;
    starRegions *= rot(radius);
    float youngStars = fbm(starRegions);
    youngStars = pow(youngStars, 3.0) * spiralArms;

    // Overall density
    float density = spiralArms * disk * 0.8 + coreGlow * 0.6;

    // Color composition - NGC 4414 colors
    vec3 col = vec3(0.0);

    // Yellow-white core
    col += vec3(1.0, 0.95, 0.8) * core * 3.0;
    col += vec3(1.0, 0.9, 0.7) * coreGlow * 1.5;

    // Orange-yellow inner disk
    col += vec3(1.0, 0.7, 0.4) * density * (1.0 - radius * 0.5);

    // Blue-white spiral arms (young stars)
    col += vec3(0.6, 0.7, 1.0) * spiralArms * disk * 0.8;

    // Blue star forming regions
    col += vec3(0.4, 0.6, 1.0) * youngStars * 2.0;

    // Add individual stars
    vec2 starCoord = p * 80.0;
    float stars = hash(floor(starCoord));
    stars = pow(stars, 100.0) * 8.0;
    col += vec3(stars);

    // Dust (reddish-brown)
    float dustColor = (1.0 - dustLanes) * spiralArms * 0.3;
    col += vec3(0.6, 0.4, 0.3) * dustColor;

    return col;
}

// Gravitational lensing - spacetime warping
vec2 warpSpacetime(vec2 uv, float time) {
    // Moving black hole / massive object
    vec2 massPos = vec2(sin(time * 0.3) * 0.4, cos(time * 0.3) * 0.3);
    vec2 toMass = uv - massPos;
    float dist = length(toMass);

    // Schwarzschild radius effect
    float mass = 0.25;
    float lensStrength = mass / (dist + 0.05);

    // Bend light rays
    float angle = atan(toMass.y, toMass.x);
    angle += lensStrength * 1.5;

    // Radial stretching
    float distortion = 1.0 + lensStrength * 0.8;
    float newDist = dist * distortion;

    vec2 warped = massPos + vec2(cos(angle), sin(angle)) * newDist;

    // Add secondary lensing object
    vec2 mass2Pos = vec2(cos(time * 0.4) * 0.5, sin(time * 0.25) * 0.4);
    vec2 toMass2 = warped - mass2Pos;
    float dist2 = length(toMass2);
    float lens2 = (mass * 0.7) / (dist2 + 0.05);

    float angle2 = atan(toMass2.y, toMass2.x);
    angle2 += lens2 * 1.2;
    float newDist2 = dist2 * (1.0 + lens2 * 0.6);

    warped = mass2Pos + vec2(cos(angle2), sin(angle2)) * newDist2;

    return warped;
}

void main() {
    vec2 uv = (fragCoord - 0.5 * ubo.iResolution.xy) / ubo.iResolution.y;
    float t = ubo.iTime * 0.2;

    // Apply spacetime warping - keep the distortion!
    vec2 warpedUV = warpSpacetime(uv, t);

    // Sample galaxy at warped coordinates
    vec3 col = galaxyPhoto(warpedUV);

    // Vignette
    float vignette = 1.0 - length(uv) * 0.4;
    vignette = smoothstep(0.2, 1.0, vignette);
    col *= vignette;

    // Tone mapping for realistic photo look
    col = col / (col + vec3(0.7));
    col = pow(col, vec3(0.8));

    fragColor = vec4(col, 1.0);
}
