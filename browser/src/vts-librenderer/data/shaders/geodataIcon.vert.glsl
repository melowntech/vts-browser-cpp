
layout(std140) uniform uboIconData
{
    mat4 uniMvp;
    vec4 uniParams;
    vec4 uniUvs;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

out vec2 varUv;

void main()
{
    varUv = mix(uniUvs.xy, uniUvs.zw, inUv);
    gl_Position = uniMvp * vec4(inPosition, 1.0);
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

