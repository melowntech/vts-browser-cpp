#version 330

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

uniform mat4 uniMvp;
uniform mat4 uniMv;
uniform mat3 uniUvMat;
uniform int uniUvSource;
uniform int uniFlatShading;

out vec3 derivativePosition;
out vec2 varUvs;

void main()
{
    gl_Position = uniMvp * vec4(inPosition, 1.0);
    if (uniFlatShading == 1)
    {
        vec4 dp = uniMv * vec4(inPosition, 1.0);
        derivativePosition = dp.xyz / dp.w;
    }
    else
    {
        switch (uniUvSource)
        {
        case 0:
            varUvs = vec2(uniUvMat * vec3(inUvInternal, 1.0));
            break;
        case 1:
            varUvs = vec2(uniUvMat * vec3(inUvExternal, 1.0));
            break;
        }
    }
}

