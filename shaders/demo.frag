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
    Code Golf Shader - Twitter Demo
    Original minified version converted and cleaned up
*/

void main() {
    vec3 r = ubo.iResolution;
    float t = ubo.iTime;
    vec4 o = vec4(0.0);

    // Normalized coordinates
    vec2 p = (fragCoord.xy * 2.0 - r.xy) / r.y / 0.3;
    vec2 v;

    // Nested loop structure (code golf style)
    for (float i = 0.0, f; i < 1e1; i++) {
        for (v = p, f = 1.0; f < 9.0; f++) {  // Start f at 1.0 to avoid div by zero
            v += sin(v.yx * f + i + t) / f;
        }
        o += (cos(i + vec4(0, 1, 2, 3)) + 1.0) / 6.0 / length(v);
    }

    // Original had o=tanh(o*0) which gives black! Fixed to show colors:
    o = tanh(o);  // Clamp to reasonable range

    fragColor = o;
}
