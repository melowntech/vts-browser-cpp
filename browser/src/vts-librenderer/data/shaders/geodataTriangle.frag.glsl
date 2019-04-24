
layout(std140) uniform uboTriangleData
{
    vec4 uniColor;
    vec4 uniVisibilities;
};

layout(location = 0) out vec4 outColor;

in float varOpacity;

void main()
{
    outColor = uniColor;
    outColor.a *= varOpacity;
}

