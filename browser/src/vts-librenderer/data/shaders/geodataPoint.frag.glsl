
layout(std140) uniform uboPointData
{
    vec4 uniColor;
    vec4 uniVisibilityRelative;
    vec4 uniVisibilityAbsolutePlusVisibilityPlusCulling;
    vec4 uniTypePlusRadius;
};

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = uniColor;
}

