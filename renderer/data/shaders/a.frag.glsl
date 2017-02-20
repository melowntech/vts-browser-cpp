#version 440

layout(binding = 0) uniform sampler2D texColor;

layout(location = 0) out vec4 outColor;

in vec2 varUvs;

void main()
{
	outColor = vec4(texture(texColor, varUvs).rgb, 1.0);
	//outColor = vec4(varUvs, 0.0, 1.0);
}

