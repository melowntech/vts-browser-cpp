
uniform mat4 uniMv;
uniform mat4 uniMvp;

uniform int uniType;

layout(location = 0) in vec3 inPosition;

void main()
{
    gl_Position = uniMvp * vec4(inPosition, 1.0);
}

