
layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilityRelative;
    vec4 uniZBufferOffsetPlusCulling;
    vec4 uniVisibilityAbsolutePlusVisibility;
    vec4 uniTypePlusUnitsPlusWidth;
};

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = uniColor;
}

