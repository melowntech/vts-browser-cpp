
layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniUnitsRadius;
};

layout(location = 0) out vec4 outColor;

in float varOpacity;

void main()
{
    if (varOpacity < 1e-7)
        discard;
    outColor = uniColor;
    outColor.a *= varOpacity;
}

