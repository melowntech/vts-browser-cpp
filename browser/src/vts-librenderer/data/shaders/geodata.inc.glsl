
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

float testVisibility(vec4 visibilities, vec3 modelPos, vec3 modelUp)
{
    vec3 pos = vec3(uniMv * vec4(modelPos, 1.0));
    float distance = length(pos);
    if (!isnan(visibilities[0]) && distance > visibilities[0])
        return 0.0;
    distance *= 2.0 / uniProj[1][1];
    if (!isnan(visibilities[1]) && distance < visibilities[1])
        return 0.0;
    if (!isnan(visibilities[2]) && distance > visibilities[2])
        return 0.0;
    vec3 up = vec3(uniMv * vec4(modelUp, 0.0));
    vec3 f = -pos / distance;
    if (!isnan(visibilities[3])
        && dot(f, up) < visibilities[3])
        return 0.0;
    return 1.0;
}
