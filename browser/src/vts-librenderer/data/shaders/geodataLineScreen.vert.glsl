
layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniUnitsRadius;
};

uniform sampler2D texLineData;

out float varOpacity;
out vec2 varCorner;

void main()
{
    bool isCap = (gl_VertexID & (1 << 31)) != 0;
    bool isEndCap = (gl_VertexID & (1 << 30)) != 0;
    int id = isCap ? (gl_VertexID & ((1 << 30) - 1)) : gl_VertexID;

    int pointIndex = id / 4 + (id + 0) % 2;
    int otherIndex = id / 4 + (id + 1) % 2;
    vec3 p = texelFetch(texLineData, ivec2(pointIndex, 0), 0).xyz;
    vec3 u = texelFetch(texLineData, ivec2(pointIndex, 1), 0).xyz;
    vec3 o = texelFetch(texLineData, ivec2(otherIndex, 0), 0).xyz;
    varOpacity = testVisibility(uniVisibilities, p, u);

    // scale
    float scale = uniUnitsRadius[1] * 2.0;
    if (int(uniUnitsRadius[0]) == 3) // ratio
        scale *= uniCameraParams[1];

    // convert to NDC space
    vec4 pp = uniMvp * vec4(p, 1.0);
    vec4 op = uniMvp * vec4(o, 1.0);
    p = pp.xyz / pp.w;
    o = op.xyz / op.w;

    // line vectors
    vec3 f = o - p;
    if ((id % 2) == 1)
        f = -f;
    f.z = 0.0;
    f = normalize(f);
    vec3 s = cross(f, vec3(0.0, 0.0, 1.0));
    if (((id % 4) / 2) == 1)
    {
        varCorner.y = -1.0;
        s = -s;
    }
    else
        varCorner.y = 1.0;
    s /= vec3(uniCameraParams.xy, 1.0);

    // caps
    varCorner.x = 0.0;
    if (isCap && (id % 2) != int(isEndCap))
    {
        if (!isEndCap)
            f = -f;
        f /= vec3(uniCameraParams.xy, 1.0);
        p = o + f * scale;
        varCorner.x = 1.0;
    }

    gl_Position = vec4(p + s * scale, 1.0);
    cullingCorrection();
}

