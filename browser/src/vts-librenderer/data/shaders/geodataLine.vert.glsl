
layout(std140) uniform uboCameraData
{
    mat4 uniProj;
    vec4 uniCameraParams; // screen height in pixels, view extent in meters
};

layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilityRelative;
    vec4 uniZBufferOffsetPlusCulling;
    vec4 uniVisibilityAbsolutePlusVisibility;
    vec4 uniTypePlusUnitsPlusWidth;
};

uniform mat4 uniMvp;
uniform mat4 uniMvpInv;
uniform mat3 uniMvInv;

uniform sampler2D texLineData;

vec3 pixelUp(vec3 a)
{
    vec4 b = uniMvp * vec4(a, 1.0);
    vec3 c = b.xyz / b.w;
    c[1] += 1.0 / uniCameraParams[0];
    vec4 d = uniMvpInv * vec4(c, 1.0);
    return d.xyz / d.w;
}

void main()
{
    int pointIndex = gl_VertexID / 4 + (gl_VertexID + 0) % 2;
    int otherIndex = gl_VertexID / 4 + (gl_VertexID + 1) % 2;
    vec3 p = texelFetch(texLineData, ivec2(pointIndex, 0), 0).xyz;
    vec3 o = texelFetch(texLineData, ivec2(otherIndex, 0), 0).xyz;
    vec3 u = vec3(0.0);
    float scale = 0.0;
    switch (int(uniTypePlusUnitsPlusWidth[0]))
    {
    case 1: // LineScreen
        u = uniMvInv * vec3(0.0, 0.0, 1.0);
        scale = length(pixelUp(p) - p); // pixels
        break;
    case 2: // LineWorld
        u = texelFetch(texLineData, ivec2(pointIndex, 1), 0).xyz;
        scale = length(u); // meters
        break;
    }
    u = normalize(u);
    if (int(uniTypePlusUnitsPlusWidth[1]) == 3) // ratio
    {
        // todo
    }
    vec3 f = normalize(o - p);
    if ((gl_VertexID % 2) == 1)
        f = -f;
    vec3 s = normalize(cross(f, u));
    if (((gl_VertexID % 4) / 2) == 1)
        s = -s;
    p += s * scale * uniTypePlusUnitsPlusWidth[2] * 0.5;
    gl_Position = uniMvp * vec4(p, 1.0);
    gl_Position[2] -= 100.0; // todo
}

