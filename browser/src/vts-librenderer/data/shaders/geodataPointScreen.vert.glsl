
layout(std140) uniform uboPointData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniUnitsRadius;
};

uniform sampler2D texPointData;

out float varOpacity;
out vec2 varCorner;

void main()
{
    int pointIndex = gl_VertexID / 4;
    int cornerIndex = gl_VertexID % 4;
    vec3 p = texelFetch(texPointData, ivec2(pointIndex, 0), 0).xyz;
    vec3 u = texelFetch(texPointData, ivec2(pointIndex, 1), 0).xyz;
    varOpacity = testVisibility(uniVisibilities, p, u);

    float scale = uniUnitsRadius[1] * 2.0;
    if (int(uniUnitsRadius[0]) == 3) // ratio
        scale *= uniCameraParams[1];

    // convert to NDC space
    vec4 pp = uniMvp * vec4(p, 1.0);
    p = pp.xyz / pp.w;
    vec3 s = vec3(1.0 / uniCameraParams[0], 0.0, 0.0);
    vec3 f = vec3(0.0, 1.0 / uniCameraParams[1], 0.0);
    switch (cornerIndex)
    {
    case 0: varCorner = vec2(-1.0, +1.0); break;
    case 1: varCorner = vec2(+1.0, +1.0); break;
    case 2: varCorner = vec2(-1.0, -1.0); break;
    case 3: varCorner = vec2(+1.0, -1.0); break;
    default: varCorner = vec2(0.0); break;
    }
    vec3 o = varCorner.x * s + varCorner.y * f;
    gl_Position = vec4(p + o * scale, 1.0);
    cullingCorrection();
}

