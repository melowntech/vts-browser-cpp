#version 330

uniform mat4 uniMvp;
uniform mat4 uniMv;
uniform mat3 uniUvMat;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvInternal;
out vec2 varUvExternal;
out vec3 derivativePosition;

void main()
{
    gl_Position = uniMvp * vec4(inPosition, 1.0);
    varUvInternal = vec2(uniUvMat * vec3(inUvInternal, 1.0));
    varUvExternal = vec2(uniUvMat * vec3(inUvExternal, 1.0));
    vec4 dp = uniMv * vec4(inPosition, 1.0);
    derivativePosition = dp.xyz / dp.w;
}

