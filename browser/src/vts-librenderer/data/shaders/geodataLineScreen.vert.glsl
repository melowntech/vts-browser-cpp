
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
    vec3 mu = texelFetch(texLineData, ivec2(pointIndex, 1), 0).xyz;
    vec3 o = texelFetch(texLineData, ivec2(otherIndex, 0), 0).xyz;
    varOpacity = testVisibility(uniVisibilities, p, mu);
    float scale = uniUnitsRadius[1];
    if (int(uniUnitsRadius[0]) == 3) // ratio
        scale *= uniCameraParams[1];

    // convert to NDC space
    vec4 pp = uniMvp * vec4(p, 1.0);
    vec4 op = uniMvp * vec4(o, 1.0);
    p = pp.xyz / pp.w;
    o = op.xyz / op.w;
    vec3 f = o - p;
    if ((gl_VertexID % 2) == 1)
        f = -f;
    f.z = 0;
    f = normalize(f);
    vec3 s = cross(f, vec3(0.0, 0.0, 1.0));
    if (((gl_VertexID % 4) / 2) == 1)
        s = -s;
    s /= vec3(uniCameraParams.xy, 1.0);
    gl_Position = vec4(p + s * scale, 1.0);

    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

