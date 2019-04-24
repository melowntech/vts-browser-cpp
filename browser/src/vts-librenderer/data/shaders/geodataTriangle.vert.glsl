
layout(std140) uniform uboTriangleData
{
    vec4 uniColor;
    vec4 uniVisibilities;
};

layout(location = 0) in vec3 inPosition;
layout(location = 0) in vec3 inUp;

out float varOpacity;

void main()
{
    varOpacity = testVisibility(uniVisibilities, inPosition, vec3(0));
    gl_Position = uniMvp * vec4(inPosition, 1.0);
}

