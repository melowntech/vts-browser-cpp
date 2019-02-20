
layout(std140) uniform uboCameraData
{
    mat4 uniProj;
    vec4 uniCameraParams; // screen width in pixels, screen height in pixels, view extent in meters
};

uniform mat4 uniMvp;
uniform vec3 uniPosition;

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
    gl_Position.xy += gl_Position.w * inPosition / uniCameraParams.xy;
}

