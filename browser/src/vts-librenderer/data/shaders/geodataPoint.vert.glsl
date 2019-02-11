
layout(std140) uniform uboCameraData
{
    mat4 uniProj;
    vec4 uniCameraParams; // screen height in pixels, view extent in meters
};

layout(std140) uniform uboPointData
{
    vec4 uniColor;
    vec4 uniVisibilityRelative;
    vec4 uniVisibilityAbsolutePlusVisibilityPlusCulling;
    vec4 uniTypePlusRadius;
};

uniform mat4 uniMvp;
uniform mat4 uniMvpInv;
uniform mat3 uniMvInv;

uniform sampler2D texPointData;

vec3 pixelUp(vec3 a)
{
    vec4 b = uniMvp * vec4(a, 1.0);
    vec3 c = b.xyz / b.w;
    c[1] += 1.0 / uniCameraParams[0];
    vec4 d = uniMvpInv * vec4(c, 1.0);
    return d.xyz / d.w;
}

vec3 pixelLeft(vec3 a)
{
    vec4 b = uniMvp * vec4(a, 1.0);
    vec3 c = b.xyz / b.w;
    c[0] += 1.0 / uniCameraParams[0];
    vec4 d = uniMvpInv * vec4(c, 1.0);
    return d.xyz / d.w;
}

void main()
{
    int pointIndex = gl_VertexID / 4;
    int cornerIndex = gl_VertexID % 4;
    vec3 p = texelFetch(texPointData, ivec2(pointIndex, 0), 0).xyz;
    vec3 u = vec3(0.0);
    float scale = 0.0;
    switch (int(uniTypePlusRadius[0]))
    {
    case 4: // PointScreen
        u = uniMvInv * vec3(0.0, 0.0, 1.0);
        scale = length(pixelUp(p) - p); // pixels
        break;
    case 5: // PointFlat
        u = texelFetch(texPointData, ivec2(pointIndex, 1), 0).xyz;
        scale = length(u); // meters
        break;
    }
    scale *= uniTypePlusRadius[1];
    u = normalize(u);
    vec3 s = normalize(pixelLeft(p) - p);
    vec3 f = normalize(cross(s, u));
    switch (gl_VertexID % 4)
    {
    case 0: p += (-s + f) * scale; break;
    case 1: p += (+s + f) * scale; break;
    case 2: p += (-s - f) * scale; break;
    case 3: p += (+s - f) * scale; break;
    }
    gl_Position = uniMvp * vec4(p, 1.0);
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

