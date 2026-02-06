#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(binding = 1) uniform sampler2D iChannel0;

// by Nikos Papadopoulos, 4rknova / 2016
	// Specular highlights contributed by Shane
	 
	#define SPECULAR
	#define SCALE 1.0
	 
	void mainImage(out vec4 col, in vec2 pc)
	{
		float time = ubo.iTime;
	    vec2 a = vec2(ubo.iResolution.x /ubo.iResolution.y, 1);
	    vec2 c = SCALE * pc.xy / ubo.iResolution.xy * a * 4. + time * .3;
	 
		float k = .1 + cos(c.y + sin(.148 - time)) + 2.4 * time;
		float w = .9 + sin(c.x + cos(.628 + time)) - 0.7 * time;
		float d = length(c);
		float s = 7. * cos(d+w) * sin(k+w);
		
		col = vec4(.5 + .5 * cos(s + vec3(.2, .5, .9)), 1);
	    
	    #ifdef SPECULAR
	    col *= vec4(1, .7, .4, 1) 
	        *  pow(max(normalize(vec3(length(dFdx(col)), 
	               length(dFdy(col)), .5/ubo.iResolution.y)).z, 0.), 2.)
	        + .75; 
	    #endif
	}