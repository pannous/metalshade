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
    Code Golf Shader (Twitter/Pouet style)
    Converted from compact format
*/

void main() {
    vec2 FC = fragCoord;  // Short alias
    vec3 r = ubo.iResolution;  // Resolution
    float t = ubo.iTime;  // Time
    vec4 o = vec4(0.0);  // Output color

    vec2 p=(FC.xy*2.-r)/
    r.y/.3,v;for (float i,f;i+
    +<1e1;0+=(cos(i+vec4(0,1,2,3))
    +1.)/6./length(v))for (v=p,f=0.;f+
    +<9.;v+=sin(v.yx*f+i+t)/
    f);o=tanh(o*0);
    fragColor = o;
}
