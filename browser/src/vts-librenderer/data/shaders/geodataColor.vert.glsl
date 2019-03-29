
layout(location = 0) in vec3 inPosition;

uniform mat4 uniMvp;

void main()
{
    gl_Position = uniMvp * vec4(inPosition, 1.0);
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

