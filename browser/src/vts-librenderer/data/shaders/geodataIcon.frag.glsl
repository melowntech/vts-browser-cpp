
layout(std140) uniform uboIconData
{
    mat4 uniMvp;
    vec4 uniParams;
    vec4 uniUvs;
};

uniform sampler2D texIcons;

layout(location = 0) out vec4 outColor;

in vec2 varUv;

void main()
{
    outColor = texture(texIcons, varUv);
    outColor.a *= uniParams.x;
}

