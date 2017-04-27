#version 440

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texMask;
layout(location = 9) uniform int uniMaskMode;
layout(location = 10) uniform int uniTexMode;
layout(location = 11) uniform float uniAlpha;

layout(location = 0) out vec4 outColor;

in vec2 varUvs;

void main()
{
	vec4 t = texture(texColor, varUvs);
	if (uniTexMode == 1)
		t = t.rrra;
	if (uniMaskMode != 0)
		t[3] *= texture(texMask, varUvs).r;
	t[3] *= uniAlpha;
	outColor = t;
}

