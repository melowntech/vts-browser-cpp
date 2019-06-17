
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
    bool isCap = (gl_VertexID & (1 << 30)) != 0;
    bool isEndCap = (gl_VertexID & (1 << 29)) != 0;
    int id = isCap ? (gl_VertexID & ((1 << 29) - 1)) : gl_VertexID;

    int pointIndex = id / 4 + (id + 0) % 2;
    int otherIndex = id / 4 + (id + 1) % 2;
    int tw = textureSize(texLineData, 0).x;
    int xx = pointIndex % tw;
    int yy = (pointIndex / tw) * 2;
    int ox = otherIndex % tw;
    int oy = (otherIndex / tw) * 2;
    vec3 p = texelFetch(texLineData, ivec2(xx, yy + 0), 0).xyz;
    vec3 u = texelFetch(texLineData, ivec2(xx, yy + 1), 0).xyz;
    vec3 o = texelFetch(texLineData, ivec2(ox, oy + 0), 0).xyz;
    varOpacity = testVisibility(uniVisibilities, p, u);

    // scale
    float scale = uniUnitsRadius[1];
    if (int(uniUnitsRadius[0]) == 3) // ratio
        scale *= uniCameraParams[2];

    // line vectors
    vec3 f = normalize(o - p);
    if ((id % 2) == 1)
        f = -f;
    vec3 s = normalize(cross(f, u));
    if (((id % 4) / 2) == 1)
    {
        varCorner.y = -1.0;
        s = -s;
    }
    else
        varCorner.y = 1.0;

    // caps
    varCorner.x = 0.0;
    if (isCap && (id % 2) != int(isEndCap))
    {
        if (!isEndCap)
            f = -f;
        p = o + f * scale;
        varCorner.x = 1.0;
    }

    gl_Position = uniMvp * vec4(p + s * scale, 1.0);
    cullingCorrection();
}

