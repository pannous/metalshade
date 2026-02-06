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
    Twitter-style Code Golf Shader
    A working example of compact shader style

    Original compact form:
    p=FC.xy/r.xy-.5;o=cos(t+vec4(0,1,2,0)+length(p)*9.-atan(p.y,p.x)*3.);
*/

void main() {
    vec3 r = ubo.iResolution;
    float t = ubo.iTime;
    vec4 o;

    // Centered UV coordinates (-0.5 to 0.5)
    vec2 p = fragCoord.xy / r.xy - 0.5;

    // Polar coordinates
    float dist = length(p);
    float angle = atan(p.y, p.x);

    // Animated color pattern
    o = cos(t + vec4(0, 1, 2, 0) + dist * 9.0 - angle * 3.0);

    // Boost brightness
    o = o * 0.5 + 0.5;

    fragColor = o;
}
