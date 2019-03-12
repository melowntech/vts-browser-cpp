
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

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in int inPlane;

out vec2 varUv;
flat out int varPlane;

void main()
{
    varUv = inUv;
    varPlane = inPlane;
    gl_Position = uniMvp * vec4(uniPosition, 1.0);
    gl_Position.xy += uniScale * gl_Position.w * inPosition / uniCameraParams.xy;
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

