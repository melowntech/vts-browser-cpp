
uniform sampler2D texColor;

in vec2 varUV;

out vec4 outColor;

void main()
{
    outColor = texture(texColor, varUV); //vec4(1.0,0.0,0.0,1.0);
    outColor.xyz = vec3((outColor.x + outColor.y + outColor.z)*0.3333);
}

