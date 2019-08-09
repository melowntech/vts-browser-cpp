
layout(std140) uniform uboIconData
{
    mat3x4 uniScreen;
    vec4 uniModelPos;
    vec4 uniColor;
    vec4 uniUvs;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

out vec2 varUv;

void main()
{
    varUv = mix(uniUvs.xy, uniUvs.zw, inUv);
    gl_Position = uniMvp * vec4(uniModelPos);
    gl_Position.xy += vec2(mat3(uniScreen) * vec3(inPosition.xy, 1.0)) * gl_Position.w;
    cullingCorrection();
}

