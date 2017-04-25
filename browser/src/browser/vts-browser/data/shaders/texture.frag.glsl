#version 440

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texMask;
layout(location = 9) uniform int uniMaskMode;
layout(location = 10) uniform int uniTexMode;

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
	vec4 t = texture(texColor, varUvs);
	if (uniTexMode == 1)
		t = t.rrra;
	outColor = t;
}

