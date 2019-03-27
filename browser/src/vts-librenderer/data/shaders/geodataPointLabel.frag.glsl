
uniform sampler2D texGlyphs;

layout(std140) uniform uboText
{
    vec4 uniColor[2];
    vec4 uniOutline;
    vec4 uniPosition; // xyz, scale
    vec4 uniCoordinates[1020];
    // 0, 1: position
    // 2, 3: uv
    // 2: + plane index (multiplied by 2)
};

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
    outColor = uniColor[1 - uniPass];
    outColor.a *= a;
}

