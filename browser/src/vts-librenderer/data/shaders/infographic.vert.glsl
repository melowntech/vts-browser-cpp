
layout(std140) uniform uboInfographics
{
    mat4 uniMvp;
    vec4 uniColor;
    ivec4 uniUseColorTexture;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvs;

void main()
{
    gl_Position = uniMvp * vec4(inPosition, 1.0);
    varUvs = inUvInternal;
}

