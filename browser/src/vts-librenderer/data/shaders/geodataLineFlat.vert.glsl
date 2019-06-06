
layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniUnitsRadius;
};

uniform sampler2D texLineData;

out float varOpacity;

void main()
{
    int pointIndex = gl_VertexID / 4 + (gl_VertexID + 0) % 2;
    int otherIndex = gl_VertexID / 4 + (gl_VertexID + 1) % 2;
    vec3 p = texelFetch(texLineData, ivec2(pointIndex, 0), 0).xyz;
    vec3 u = texelFetch(texLineData, ivec2(pointIndex, 1), 0).xyz;
    vec3 o = texelFetch(texLineData, ivec2(otherIndex, 0), 0).xyz;
    varOpacity = testVisibility(uniVisibilities, p, u);
    float scale = length(u) * uniUnitsRadius[1];
    u = normalize(u);
    if (int(uniUnitsRadius[0]) == 3) // ratio
        scale *= uniCameraParams[2];
    vec3 f = normalize(o - p);
    if ((gl_VertexID % 2) == 1)
        f = -f;
    vec3 s = normalize(cross(f, u));
    if (((gl_VertexID % 4) / 2) == 1)
        s = -s;
    p += s * scale;
    gl_Position = uniMvp * vec4(p, 1.0);
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

