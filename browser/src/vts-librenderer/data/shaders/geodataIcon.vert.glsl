
layout(std140) uniform uboIconData
{
    mat4 uniScreen;
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
    gl_Position += (uniScreen * vec4(inPosition, 1.0)) * gl_Position.w;
    cullingCorrection();
}

