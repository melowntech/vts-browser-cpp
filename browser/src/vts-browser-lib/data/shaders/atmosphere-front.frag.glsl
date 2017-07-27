#version 330

uniform vec4 uniColorLow;
uniform vec4 uniColorHigh;
uniform vec3 uniBodyRadiuses; // major, minor, atmosphere thickness

uniform sampler2D texDepth;
uniform sampler2D texColor;

in vec2 varUv;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(0,0,0,0);
}

