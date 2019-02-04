
layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilityRelative;
    vec4 uniZBufferOffsetPlusCulling;
    vec4 uniVisibilityAbsolutePlusVisibility;
    vec4 uniTypePlusUnitsPlusWidth;
};

uniform mat4 uniMvp;

uniform sampler2D texLineData;

void main()
{
    int pointIndex = gl_VertexID / 4 + (gl_VertexID + 0) % 2;
    int otherIndex = gl_VertexID / 4 + (gl_VertexID + 1) % 2;
    vec3 p = texelFetch(texLineData, ivec2(pointIndex, 0), 0).xyz;
    vec3 o = texelFetch(texLineData, ivec2(otherIndex, 0), 0).xyz;
    vec3 u = texelFetch(texLineData, ivec2(pointIndex, 1), 0).xyz;
    float meter = u.length();
    u = normalize(u);
    vec3 f = normalize(o - p);
    if ((gl_VertexID % 2) == 1)
        f = -f;
    vec3 s = normalize(cross(f, u));
    if (((gl_VertexID % 4) / 2) == 1)
        s = -s;
    p += s * meter * uniTypePlusUnitsPlusWidth[2] * 0.5;
    gl_Position = uniMvp * vec4(p, 1.0);
}

