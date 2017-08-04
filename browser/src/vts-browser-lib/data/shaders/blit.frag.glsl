#version 330

uniform sampler2D texColor;

layout(location = 0) out vec4 outColor;

in vec2 varUvs;

void main()
{
    outColor = texture(texColor, varUvs);
}

