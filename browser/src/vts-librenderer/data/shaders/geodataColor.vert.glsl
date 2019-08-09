
layout(std140) uniform uboColorData
{
    mat3x4 uniScreen;
    vec4 uniModelPos;
    vec4 uniColor;
};

layout(location = 0) in vec3 inPosition;

void main()
{
    gl_Position = uniMvp * vec4(uniModelPos);
    gl_Position.xy += vec2(mat3(uniScreen) * vec3(inPosition.xy, 1.0)) * gl_Position.w;
    cullingCorrection();
}

