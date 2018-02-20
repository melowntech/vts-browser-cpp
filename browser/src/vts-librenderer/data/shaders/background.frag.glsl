
in vec3 varFragDir;

layout(location = 0) out vec4 outColor;

void main()
{
    float atmosphere = atmDensityDir(varFragDir, 1001.0);
    outColor = atmColor(atmosphere, vec4(0.0, 0.0, 0.0, 1.0));
}

