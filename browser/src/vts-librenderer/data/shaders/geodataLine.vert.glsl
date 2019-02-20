
layout(std140) uniform uboCameraData
{
    mat4 uniProj;
    vec4 uniCameraParams; // screen width in pixels, screen height in pixels, view extent in meters
};

layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilityRelative;
    vec4 uniVisibilityAbsolutePlusVisibilityPlusCulling;
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
    case 2: // LineFlat
        u = texelFetch(texLineData, ivec2(pointIndex, 1), 0).xyz;
        scale = length(u); // meters
        break;
    }
    u = normalize(u);
    if (int(uniTypePlusUnitsPlusWidth[1]) == 3) // ratio
    {
        switch (int(uniTypePlusUnitsPlusWidth[0]))
        {
        case 1: // LineScreen
            scale *= uniCameraParams[1]; // screen height in pixels
            break;
        case 2: // LineFlat
            scale *= uniCameraParams[2]; // view extent in meters
            break;
        }
    }
    vec3 f = normalize(o - p);
    if ((gl_VertexID % 2) == 1)
        f = -f;
    vec3 s = normalize(cross(f, u));
    if (((gl_VertexID % 4) / 2) == 1)
        s = -s;
    p += s * scale * uniTypePlusUnitsPlusWidth[2] * 0.5;
    gl_Position = uniMvp * vec4(p, 1.0);
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

