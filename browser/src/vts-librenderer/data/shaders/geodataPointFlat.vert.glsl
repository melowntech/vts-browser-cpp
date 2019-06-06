
layout(std140) uniform uboPointData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniUnitsRadius;
};

uniform sampler2D texPointData;

out float varOpacity;

// returns position, in model space, that projects into a pixel one left
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
    vec3 u = texelFetch(texPointData, ivec2(pointIndex, 1), 0).xyz;
    varOpacity = testVisibility(uniVisibilities, p, u);
    float scale = length(u) * uniUnitsRadius[1];
    if (int(uniUnitsRadius[0]) == 3) // ratio
        scale *= uniCameraParams[2];
    u = normalize(u);

    vec3 s = normalize(pixelLeft(p) - p);
    vec3 f = normalize(cross(s, u));
    vec3 o = vec3(0.0);
    switch (cornerIndex)
    {
    case 0: o = -s + f; break;
    case 1: o = +s + f; break;
    case 2: o = -s - f; break;
    case 3: o = +s - f; break;
    }
    gl_Position = uniMvp * vec4(p + o * scale, 1.0);

    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

