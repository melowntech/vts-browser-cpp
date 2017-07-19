#version 330

uniform sampler2D texColor;
uniform sampler2D texMask;

uniform int uniMaskMode;
uniform int uniTexMode;
uniform float uniAlpha;

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

