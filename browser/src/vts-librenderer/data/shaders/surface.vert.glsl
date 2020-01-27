
layout(std140) uniform uboSurface
{
    mat4 uniP;
    mat4 uniMv;
    mat3x4 uniUvMat; // + blendingCoverage
    vec4 uniUvClip;
    vec4 uniColor;
    ivec4 uniFlags; // mask, monochromatic, flat shading, uv source, lodBlendingWithDithering, ... frameIndex
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvTex;
#ifdef VTS_NO_CLIP
out vec2 varUvExternal;
#endif
out vec3 varViewPosition;
#ifdef VTS_ATM_PER_VERTEX
out float varAtmDensity;
#endif

#ifndef VTS_NO_CLIP
out float gl_ClipDistance[4];
#endif

bool getFlag(int i)
{
    return (uniFlags[i / 8] & (1 << (i % 8))) != 0;
}

void main()
{
#ifdef VTS_NO_CLIP
    varUvExternal = inUvExternal;
#else
    gl_ClipDistance[0] = (inUvExternal[0] - uniUvClip[0]) * +1.0;
    gl_ClipDistance[1] = (inUvExternal[1] - uniUvClip[1]) * +1.0;
    gl_ClipDistance[2] = (inUvExternal[0] - uniUvClip[2]) * -1.0;
    gl_ClipDistance[3] = (inUvExternal[1] - uniUvClip[3]) * -1.0;
#endif

    vec4 vp = uniMv * vec4(inPosition, 1.0);
    gl_Position = uniP * vp;
    varUvTex = vec2(mat3(uniUvMat) * vec3(getFlag(3)
            ? inUvExternal : inUvInternal, 1.0));
    varViewPosition = vp.xyz;
#ifdef VTS_ATM_PER_VERTEX
    varAtmDensity = atmDensity(varViewPosition);
#endif
}

