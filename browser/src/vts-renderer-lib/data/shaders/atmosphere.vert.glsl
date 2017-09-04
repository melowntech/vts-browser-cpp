#version 330

uniform vec3 uniCameraDirections[4];
uniform int uniProjected;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

out vec2 varUv;
out vec3 varCamDir;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    varUv = inUv;
    float x = uniProjected == 1 ? 0.5 : varUv.x;
    varCamDir = mix(
        mix(uniCameraDirections[0], uniCameraDirections[1], x),
        mix(uniCameraDirections[2], uniCameraDirections[3], x),
        varUv.y);
}

