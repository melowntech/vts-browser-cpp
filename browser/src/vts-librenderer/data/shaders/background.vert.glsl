
uniform vec3 uniCorners[4];

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

out vec3 varFragDir;

void main()
{
    gl_Position = vec4(inPosition.xy, 1.0, 1.0);
    varFragDir = mix(
        mix(uniCorners[0], uniCorners[1], inUv.x),
        mix(uniCorners[2], uniCorners[3], inUv.x), inUv.y);
}

