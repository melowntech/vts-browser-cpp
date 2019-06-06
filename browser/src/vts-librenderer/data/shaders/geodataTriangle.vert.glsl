
layout(std140) uniform uboTriangleData
{
    vec4 uniColor;
    vec4 uniVisibilities;
    int uniShading;
};

layout(location = 0) in vec3 inPosition;

out float varOpacity;
out vec3 varViewPosition;

void main()
{
    varViewPosition = (uniMv * vec4(inPosition, 1.0)).xyz;
    varOpacity = testVisibility(uniVisibilities, inPosition, vec3(0.0));
    gl_Position = uniMvp * vec4(inPosition, 1.0);
}

