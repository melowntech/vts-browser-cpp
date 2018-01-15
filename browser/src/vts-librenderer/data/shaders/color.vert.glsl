
layout(location = 0) in vec3 inPosition;

uniform mat4 uniMvp;

void main()
{
    gl_Position = uniMvp * vec4(inPosition, 1.0);
}

