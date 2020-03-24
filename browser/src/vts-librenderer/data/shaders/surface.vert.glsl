
layout(std140) uniform uboSurface
{
    mat4 uniP;
    mat4 uniMv;
    mat3x4 uniUvMat; // + blendingCoverage
    vec4 uniColor;
    ivec4 uniFlags; // mask, monochromatic, flat shading, uv source, lodBlendingWithDithering, ... frameIndex
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvTex;
out vec3 varViewPosition;
#ifdef VTS_ATM_PER_VERTEX
out float varAtmDensity;
#endif

bool getFlag(int i)
{
    return (uniFlags[i / 8] & (1 << (i % 8))) != 0;
}

void main()
{
    vec4 vp = uniMv * vec4(inPosition, 1.0);
    gl_Position = uniP * vp;
    varUvTex = vec2(mat3(uniUvMat) * vec3(getFlag(3)
            ? inUvExternal : inUvInternal, 1.0));
    varViewPosition = vp.xyz;
#ifdef VTS_ATM_PER_VERTEX
    varAtmDensity = atmDensity(varViewPosition);
#endif
}

