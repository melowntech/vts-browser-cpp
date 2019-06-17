
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

#ifdef VTS_STAGE_VERTEX

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
    if (!isnan(visibilities[3]))
    {
        vec3 up = vec3(uniMv * vec4(modelUp, 0.0));
        if (dot(normalize(-pos), normalize(up)) < visibilities[3])
            return 0.0;
    }
    return 1.0;
}

void cullingCorrection()
{
    // avoid culling geodata by near camera plane
    gl_Position.z = max(gl_Position.z, -gl_Position.w + 1e-7);
}

#endif
