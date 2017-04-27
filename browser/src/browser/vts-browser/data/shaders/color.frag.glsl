#version 440

layout(location = 8) uniform vec4 uniColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = uniColor;
}

