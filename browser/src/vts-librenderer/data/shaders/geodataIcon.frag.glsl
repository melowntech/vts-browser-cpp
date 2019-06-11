
layout(std140) uniform uboIconData
{
    mat4 uniScreen;
    vec4 uniModelPos;
    vec4 uniColor;
    vec4 uniUvs;
};

uniform sampler2D texIcons;

layout(location = 0) out vec4 outColor;

in vec2 varUv;

void main()
{
    outColor = uniColor * texture(texIcons, varUv);
}

