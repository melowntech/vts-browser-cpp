
uniform sampler2D texNormal;

in vec2 varUV;

out vec4 outColor;

void main()
{
    outColor = texture(texNormal, varUV); //vec4(1.0,0.0,0.0,1.0);
    outColor.xyz = vec3(outColor);
}

