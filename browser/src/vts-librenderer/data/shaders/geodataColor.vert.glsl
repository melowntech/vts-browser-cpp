
layout(std140) uniform uboColorData
{
    mat4 uniScreen;
    vec4 uniModelPos;
    vec4 uniColor;
};

layout(location = 0) in vec3 inPosition;

void main()
{
    gl_Position = uniMvp * vec4(uniModelPos);
    gl_Position += (uniScreen * vec4(inPosition, 1.0)) * gl_Position.w;
    cullingCorrection();
}

