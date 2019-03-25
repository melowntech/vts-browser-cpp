
uniform sampler2D texGlyphs;

uniform vec4 uniOutline[3];
uniform int uniPass;
uniform float uniOpacity;

in vec2 varUv;
flat in int varPlane;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 t4 = texture(texGlyphs, varUv);
    float t = t4[varPlane];
    float c = uniOutline[2][uniPass];
    float d = uniOutline[2][uniPass + 2];
    float a = smoothstep(c - d, c + d, t);
    outColor = uniOutline[1 - uniPass];
    outColor.a *= a * uniOpacity;
}

