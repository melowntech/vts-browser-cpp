
layout(std140) uniform uboCameraData
{
    mat4 uniProj;
    vec4 uniCameraParams; // screen width in pixels, screen height in pixels, view extent in meters
};

layout(std140) uniform uboViewData
{
    mat4 uniMvp;
    mat4 uniMvpInv;
    mat4 uniMv;
    mat4 uniMvInv;
};

layout(std140) uniform uboPointData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniTypePlusRadius;
};

uniform sampler2D texPointData;

out float varOpacity;

float testVisibility(vec3 modelPos, vec3 modelUp)
{
    vec3 pos = vec3(uniMv * vec4(modelPos, 1.0));
    float distance = length(pos);
    if (uniVisibilities[0] == uniVisibilities[0] && distance > uniVisibilities[0])
        return 0.0;
    distance *= 2.0 / uniProj[1][1];
    if (uniVisibilities[1] == uniVisibilities[1] && distance < uniVisibilities[1])
        return 0.0;
    if (uniVisibilities[2] == uniVisibilities[2] && distance > uniVisibilities[2])
        return 0.0;
    vec3 up = vec3(uniMv * vec4(modelUp, 0.0));
    vec3 f = normalize(-pos);
    if (uniVisibilities[3] == uniVisibilities[3]
        && dot(f, up) < uniVisibilities[3])
        return 0.0;
    return 1.0;
}

vec3 pixelUp(vec3 a)
{
    vec4 b = uniMvp * vec4(a, 1.0);
    vec3 c = b.xyz / b.w;
    c[1] += 1.0 / uniCameraParams[1];
    vec4 d = uniMvpInv * vec4(c, 1.0);
    return d.xyz / d.w;
}

vec3 pixelLeft(vec3 a)
{
    vec4 b = uniMvp * vec4(a, 1.0);
    vec3 c = b.xyz / b.w;
    c[0] += 1.0 / uniCameraParams[1];
    vec4 d = uniMvpInv * vec4(c, 1.0);
    return d.xyz / d.w;
}

void main()
{
    int pointIndex = gl_VertexID / 4;
    int cornerIndex = gl_VertexID % 4;
    vec3 p = texelFetch(texPointData, ivec2(pointIndex, 0), 0).xyz;
    vec3 mu = texelFetch(texPointData, ivec2(pointIndex, 1), 0).xyz;
    varOpacity = testVisibility(p, mu);
    vec3 u = vec3(0.0);
    float scale = 0.0;
    switch (int(uniTypePlusRadius[0]))
    {
    case 4: // PointScreen
        u = mat3(uniMvInv) * vec3(0.0, 0.0, 1.0);
        scale = length(pixelUp(p) - p); // pixels
        break;
    case 5: // PointFlat
        u = mu;
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

