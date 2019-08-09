
layout(std140) uniform uboText
{
    vec4 uniColor[2];
    vec4 uniOutline;
    vec4 uniPosition; // xyz, scale
    vec4 uniOffset;
    vec4 uniCoordinates[1000];
    // 0, 1: position
    // 2, 3: uv
    // 2: + plane index (multiplied by 2)
};

out vec2 varUv;
flat out int varPlane;

void main()
{
    // quad
    // 2--3
    // |  |
    // 0--1
    // triangles
    // 0-1-2
    // 1-3-2
    int tri = (gl_VertexID / 3) % 2;
    int vert = gl_VertexID % 3;
    int coordId = tri == 0 ? vert :
        vert == 0 ? 1 : vert == 1 ? 3 : 2;
    vec4 coord = uniCoordinates[coordId + 4 * (gl_VertexID / 6)];
    vec2 pos = coord.xy;
    varPlane = int(coord.z) / 2;
    varUv = coord.zw;
    varUv.x -= float(varPlane * 2);
    gl_Position = uniMvp * vec4(uniPosition.xyz, 1.0);
    gl_Position.xy += gl_Position.w * uniOffset.xy;
    gl_Position.xy += gl_Position.w * uniPosition.w
                        * pos / uniCameraParams.xy;
    cullingCorrection();
}

