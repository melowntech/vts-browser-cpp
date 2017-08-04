#version 330

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvs;

out vec2 varUvs;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    varUvs = inUvs;
}

