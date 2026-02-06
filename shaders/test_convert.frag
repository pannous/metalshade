#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;
// layout(binding = 2) uniform sampler2D iChannel1;
// layout(binding = 3) uniform sampler2D iChannel2;
// layout(binding = 4) uniform sampler2D iChannel3;

// If you intend to reuse this shader, please add credits to 'Danilo Guanabara'

void main(){
	vec3 c;
	float l,z=ubo.iTime;
	for(int i=0;i<3;i++) {
		vec2 uv,p=fragCoord.xy/ubo.iResolution.xy;
		uv=p;
		p-=.5;
		p.x*=ubo.iResolution.xy.x/ubo.iResolution.xy.y;
		z+=.07;
		l=length(p);
		uv+=p/l*(sin(z)+1.)*abs(sin(l*9.-z-z));
		c[i]=.01/length(mod(uv,1.)-.5);
	}
	fragColor=vec4(c/l,ubo.iTime);
}
