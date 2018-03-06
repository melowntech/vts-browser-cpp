
uniform mat4 uniP;
uniform mat4 uniMv;
uniform mat3 uniUvMat;
uniform ivec4 uniFlags; // mask, monochromatic, flat shading, uv source

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvTex;
out vec2 varUvClip;
out vec3 varViewPosition;

void main()
{
    vec4 vp = uniMv * vec4(inPosition, 1.0);
    gl_Position = uniP * vp;
    varUvTex = vec2(uniUvMat * vec3(uniFlags.w > 0 ? inUvExternal : inUvInternal, 1.0));
    varUvClip = inUvExternal;
    varViewPosition = vp.xyz;
}

