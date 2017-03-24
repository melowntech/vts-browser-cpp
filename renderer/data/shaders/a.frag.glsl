#version 440

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texMask;
layout(location = 9) uniform int uniMaskMode;

layout(location = 0) out vec4 outColor;

in vec2 varUvs;

void main()
{
	if (uniMaskMode != 0)
	{
		float val = texture(texMask, varUvs).r;
		if (val < 0.5)
			discard;
	}
	outColor = vec4(texture(texColor, varUvs).rgb, 1.0);
}

