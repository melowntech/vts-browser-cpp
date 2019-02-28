
layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniTypePlusUnitsPlusWidth;
};

layout(location = 0) out vec4 outColor;

in float varOpacity;

void main()
{
    outColor = uniColor;
    outColor.a *= varOpacity;
}

