

layout(std140) uniform uboPointLabelData
{
    vec4 uniColors[2];
    vec4 uniOutline;
};

uniform sampler2D texGlyphs;

uniform int uniPass;

in vec2 varUv;
flat in int varPlane;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 t4 = texture(texGlyphs, varUv);
    float t = t4[varPlane];
    float c = uniOutline[uniPass];
    float d = uniOutline[uniPass + 2];
    float a = smoothstep(c - d, c + d, t);
    outColor = uniColors[1 - uniPass];
    outColor.a *= a;
}

