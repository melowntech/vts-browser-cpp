
layout(std140) uniform uboPointData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniUnitsRadius;
};

layout(location = 0) out vec4 outColor;

in float varOpacity;
in vec2 varCorner;

void main()
{
    if (length(varCorner) >= 1.0 || varOpacity < 0.01)
        discard; // prevent the corners from masking (via stencil test) other geodata
    outColor = uniColor;
    outColor.a *= varOpacity;
}

