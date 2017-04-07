#version 440

layout(location = 8) uniform vec3 uniColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(uniColor, 1.0);
}

