
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

uniform vec3 uniPosition;
uniform float uniScale;

// 0, 1: position
// 2, 3: uv
// 2: + plane index (multiplied by 2)
uniform vec4 uniCoordinates[256];

out vec2 varUv;
flat out int varPlane;

void main()
{
    vec2 pos = uniCoordinates[gl_VertexID].xy;
    varPlane = int(uniCoordinates[gl_VertexID].z) / 2;
    varUv = uniCoordinates[gl_VertexID].zw;
    varUv.x -= float(varPlane * 2);
    gl_Position = uniMvp * vec4(uniPosition, 1.0);
    gl_Position.xy += uniScale * gl_Position.w * pos / uniCameraParams.xy;
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

