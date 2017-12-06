
uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 Out_Color;

void main()
{
    ivec2 res = textureSize(Texture, 0);
    Out_Color = Frag_Color * texture(Texture, Frag_UV.st + 0.5 / res);
}

