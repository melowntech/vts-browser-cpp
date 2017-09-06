#version 330

uniform mat4 uniMvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvs;

void main()
{
    gl_Position = uniMvp * vec4(inPosition, 1.0);
    varUvs = inUvInternal;
}

