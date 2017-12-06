
uniform sampler2D uniTexture;

in vec2 varUv;
out vec4 outColor;

void main()
{
    outColor = texture(uniTexture, varUv);
}

