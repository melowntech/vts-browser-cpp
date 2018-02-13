
uniform vec3 uniCornerDirs[4];

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

out vec3 varCfd; // camera fragment direction
flat out vec3 varCvd; // camera view direction

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    varCfd = normalize(mix(
        mix(uniCornerDirs[0], uniCornerDirs[1], inUv.x),
        mix(uniCornerDirs[2], uniCornerDirs[3], inUv.x), inUv.y));
    varCvd = normalize(mix(
        mix(uniCornerDirs[0], uniCornerDirs[1], 0.5),
        mix(uniCornerDirs[2], uniCornerDirs[3], 0.5), 0.5));
}

