
layout(std140) uniform uboColorData
{
    mat3x4 uniScreen;
    vec4 uniModelPos;
    vec4 uniColor;
};

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = uniColor;
}

