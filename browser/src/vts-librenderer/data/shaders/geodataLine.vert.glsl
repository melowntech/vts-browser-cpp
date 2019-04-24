
layout(std140) uniform uboLineData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    vec4 uniTypePlusUnitsPlusWidth;
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
    vec3 u = vec3(0.0);
    float scale = 0.0;
    switch (int(uniTypePlusUnitsPlusWidth[0]))
    {
    case 1: // LineScreen
        u = mat3(uniMvInv) * vec3(0.0, 0.0, 1.0);
        scale = length(pixelUp(p) - p); // pixels
        break;
    case 2: // LineFlat
        u = mu;
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

