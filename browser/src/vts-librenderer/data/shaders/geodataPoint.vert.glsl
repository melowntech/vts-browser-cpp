
layout(std140) uniform uboPointData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniTypePlusRadius;
};

uniform sampler2D texPointData;

out float varOpacity;

void main()
{
    int pointIndex = gl_VertexID / 4;
    int cornerIndex = gl_VertexID % 4;
    vec3 p = texelFetch(texPointData, ivec2(pointIndex, 0), 0).xyz;
    vec3 mu = texelFetch(texPointData, ivec2(pointIndex, 1), 0).xyz;
    varOpacity = testVisibility(uniVisibilities, p, mu);
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

