
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
    float u = length(abs(varCorner));
    float du = fwidth(u);
    if (u >= 1.0)
        discard; // prevent the corners from masking (via stencil test) other geodata
    if (varOpacity < 1e-7)
        discard;
    outColor = uniColor;
    outColor.a *= varOpacity * (1.0 - smoothstep(1.0 - du, 1.0, u));
}

