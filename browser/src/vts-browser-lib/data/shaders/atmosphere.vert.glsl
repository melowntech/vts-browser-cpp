#version 330

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

out vec2 varUv;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
	varUv = inUv;
}

