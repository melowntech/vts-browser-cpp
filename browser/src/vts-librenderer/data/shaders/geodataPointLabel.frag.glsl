
uniform sampler2D texGlyphs;

in vec2 varUv;
flat in int varPlane;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 t4 = texture(texGlyphs, varUv);
    float t = t4[varPlane];
    outColor = vec4(t, t, t, 1.0);
}

