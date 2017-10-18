
uniform mat4 uniMvp;
uniform mat3 uniUvm;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;

out vec2 varUv;

void main()
{
    varUv = (uniUvm * vec3(inUv, 1)).xy;
    gl_Position = uniMvp * vec4(inPos, 1);
}

