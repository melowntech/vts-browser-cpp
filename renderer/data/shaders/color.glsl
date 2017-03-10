$vertex
#version 440

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

layout(location = 0) uniform mat4 uniMvp;

void main()
{
	gl_Position = uniMvp * vec4(inPosition, 1.0);
}

$fragment
#version 440

layout(location = 8) uniform vec3 uniColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(uniColor, 1.0);
}

