
layout(std140) uniform uboCameraData
{
    mat4 uniProj;
    vec4 uniCameraParams; // screen width in pixels, screen height in pixels, view extent in meters
};

layout(std140) uniform uboViewData
{
    mat4 uniMvp;
    mat4 uniMvpInv;
    mat4 uniMv;
    mat4 uniMvInv;
};

vec3 pixelUp(vec3 a)
{
    vec4 b = uniMvp * vec4(a, 1.0);
    vec3 c = b.xyz / b.w;
    c[1] += 1.0 / uniCameraParams[1];
    vec4 d = uniMvpInv * vec4(c, 1.0);
    return d.xyz / d.w;
}

vec3 pixelLeft(vec3 a)
{
    vec4 b = uniMvp * vec4(a, 1.0);
    vec3 c = b.xyz / b.w;
    c[0] += 1.0 / uniCameraParams[1];
    vec4 d = uniMvpInv * vec4(c, 1.0);
    return d.xyz / d.w;
}

float testVisibility(vec4 visibilities, vec3 modelPos, vec3 modelUp)
{
    vec3 pos = vec3(uniMv * vec4(modelPos, 1.0));
    float distance = length(pos);
    if (visibilities[0] == visibilities[0] && distance > visibilities[0])
        return 0.0;
    distance *= 2.0 / uniProj[1][1];
    if (visibilities[1] == visibilities[1] && distance < visibilities[1])
        return 0.0;
    if (visibilities[2] == visibilities[2] && distance > visibilities[2])
        return 0.0;
    vec3 up = vec3(uniMv * vec4(modelUp, 0.0));
    vec3 f = -pos / distance;
    if (visibilities[3] == visibilities[3]
        && dot(f, up) < visibilities[3])
        return 0.0;
    return 1.0;
}
