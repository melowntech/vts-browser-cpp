
layout(location = 0) in vec3 inPos;

out vec2 varUV;

void main()
{
    varUV = (inPos.xy + 1.0) * 0.5;
    gl_Position = vec4(inPos, 1);
}

