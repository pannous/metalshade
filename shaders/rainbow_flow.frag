#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

// Rotate 2D
mat2 rot(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

// Rainbow color palette
vec3 rainbow(float t) {
    t = fract(t);
    float r = abs(t * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(t * 6.0 - 2.0);
    float b = 2.0 - abs(t * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

// Smooth noise
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

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

// Fractal brownian motion
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

void main() {
    vec2 uv = (fragCoord - 0.5 * ubo.iResolution.xy) / ubo.iResolution.y;
    float t = ubo.iTime * 0.5;

    // Create kaleidoscope effect
    float angle = atan(uv.y, uv.x);
    float radius = length(uv);

    // Mirror kaleidoscope segments
    float segments = 6.0;
    angle = mod(angle, 6.28318 / segments);
    angle = abs(angle - 3.14159 / segments);

    // Recreate coordinates from polar
    vec2 p = vec2(cos(angle), sin(angle)) * radius;

    // Add rotation
    p *= rot(t * 0.3);

    // Create flowing patterns
    vec2 q = p * 3.0;
    q += vec2(fbm(p * 2.0 + t), fbm(p * 2.0 - t)) * 0.5;

    // Multiple layers of flow
    float flow1 = fbm(q + t * 0.8);
    float flow2 = fbm(q * 1.5 - t * 0.6);
    float flow3 = fbm(q * 2.0 + vec2(t, -t) * 0.4);

    // Combine flows
    float pattern = flow1 * 0.5 + flow2 * 0.3 + flow3 * 0.2;

    // Add spiral
    float spiral = angle * 2.0 + radius * 5.0 - t * 2.0;
    pattern += sin(spiral) * 0.3;

    // Add concentric rings
    float rings = sin(radius * 10.0 - t * 3.0) * 0.5 + 0.5;
    pattern = mix(pattern, rings, 0.3);

    // Create rainbow colors
    float colorShift = pattern + t * 0.3 + radius * 0.5;
    vec3 col = rainbow(colorShift);

    // Add secondary rainbow layer
    vec3 col2 = rainbow(colorShift + 0.5 + sin(t) * 0.2);
    col = mix(col, col2, sin(pattern * 6.28318 + t) * 0.5 + 0.5);

    // Add brightness variation
    float brightness = 0.6 + pattern * 0.4;
    brightness += sin(radius * 8.0 - t * 2.0) * 0.2;
    col *= brightness;

    // Add glow at center
    float centerGlow = 1.0 / (1.0 + radius * radius * 2.0);
    col += rainbow(t * 0.5) * centerGlow * 0.5;

    // Add sparkles
    float sparkle = noise(p * 20.0 + t * 2.0);
    sparkle = pow(sparkle, 20.0);
    col += vec3(sparkle) * 2.0;

    // Vignette
    float vignette = 1.0 - radius * 0.6;
    vignette = smoothstep(0.0, 1.0, vignette);
    col *= vignette;

    // Contrast and saturation
    col = col * col * (3.0 - 2.0 * col); // Smooth contrast
    col = pow(col, vec3(0.9)); // Slight gamma

    fragColor = vec4(col, 1.0);
}
