
uniform sampler2D texAtmDensity;

layout(std140) uniform uboAtm
{
    mat4 uniAtmViewInv;
    vec4 uniAtmColorLow;
    vec4 uniAtmColorHigh;
    vec4 uniAtmParams; // atmosphere thickness, horizontal exponent, minor-axis (divided by major axis), major-axis
    vec3 uniAtmCameraPosition; // world position of camera (divided by major semi-axis)
};

float atmDecodeFloat(vec4 rgba)
{
    return dot(rgba, vec4(1.0, 1.0 / 256.0, 1.0 / (256.0*256.0), 1.0 / (256.0*256.0*256.0)));
}

float atmSampleDensity(vec2 uv)
{
    // since some color channels of the density texture are not continuous
    //   it is important to first decode the float from rgba and only after
    //   that to filter the texture
    ivec2 res = textureSize(texAtmDensity, 0);
    vec2 uvp = uv * vec2(res - 1);
    ivec2 iuv = ivec2(uvp); // upper-left texel fetch coordinates
    vec4 s;
    s.x = atmDecodeFloat(texelFetchOffset(texAtmDensity, iuv, 0, ivec2(0,0)));
    s.y = atmDecodeFloat(texelFetchOffset(texAtmDensity, iuv, 0, ivec2(1,0)));
    s.z = atmDecodeFloat(texelFetchOffset(texAtmDensity, iuv, 0, ivec2(0,1)));
    s.w = atmDecodeFloat(texelFetchOffset(texAtmDensity, iuv, 0, ivec2(1,1)));
    vec2 f = fract(uvp); // interpolation factors
    vec2 a = mix(s.xz, s.yw, f.x);
    float b = mix(a.x, a.y, f.y);
    return b * 5.0;
}

// fragVect is in world space (divided by major axis)
float atmDensityDir(vec3 fragDir, float fragDist)
{
    if (uniAtmParams[0] == 0.0) // no atmosphere
        return 0.0;

    // convert from ellipsoidal into spherical space
    vec3 ellipseToSphere = vec3(1.0, 1.0, 1.0 / uniAtmParams[2]);
    vec3 camPos = uniAtmCameraPosition * ellipseToSphere;
    vec3 camNormal = normalize(camPos);
    fragDir = normalize(fragDir * ellipseToSphere);
    if (fragDist < 1000.0)
    {
        vec3 T = uniAtmCameraPosition + fragDist * fragDir;
        T *= ellipseToSphere;
        fragDist = length(T - camPos);
    }

    // ray parameters
    float ts[2];
    ts[1] = fragDist; // max ray length
    float l = length(camPos); // distance of camera center from world origin
    float x = dot(fragDir, -camNormal) * l; // distance from camera to a point called "x", which is on the ray and closest to world origin
    float y2 = l * l - x * x;
    float y = sqrt(y2); // distance of the ray from world origin

    float atmHeight = uniAtmParams[0]; // atmosphere height (excluding planet radius)
    float atmRad = 1.0 + atmHeight; // atmosphere height including planet radius
    float atmRad2 = atmRad * atmRad;

    if (y > atmRad)
        return 0.0; // the ray does not cross the atmosphere

    float t1e = x - sqrt(1.0 - y2); // t1 at ellipse
    
    // fill holes in terrain if the ray passes through the planet
    if (y < 0.998 && x >= 0.0 && ts[1] > 1000.0)
        ts[1] = t1e;

    // approximate the planet by the ellipsoid if the mesh is too rough
    if (y <= 1.0)
        ts[1] = mix(ts[1], t1e, clamp((l - 1.4) / 0.1, 0.0, 1.0));

    // to improve accuracy, swap direction of the ray to point out of the terrain
    bool swapDirection = ts[1] < 1000.0 && x >= 0.0;

    // distance of atmosphere boundary from "x"
    float a = sqrt(atmRad2 - y2);

    // clamp t0 and t1 to atmosphere boundaries
    // ts is line segment that encloses the unobstructed portion of the ray and is inside atmosphere
    ts[0] = max(0.0, x - a);
    ts[1] = min(ts[1], x + a);

    // sample the density texture
    float ds[2];
    for (int i = 0; i < 2; i++)
    {
        float t = x - ts[i];
        float r = sqrt(t * t + y2);
        vec2 uv = vec2(0.5 - 0.5 * t / r, 0.5 + 0.5 * (r - 1.0) / atmHeight);
        if (swapDirection)
            uv.x = 1.0 - uv.x;
        ds[i] = atmSampleDensity(uv);
    }

    // final optical transmittance
    float density = ds[0] - ds[1];
    if (swapDirection)
        density *= -1.0;
    return 1.0 - exp(-uniAtmParams[1] * density);
}

// fragVect is view-space fragment position
float atmDensity(vec3 fragVect)
{
    // convert fragVect to world-space and divide by major radius
    fragVect = (uniAtmViewInv * vec4(fragVect, 1.0)).xyz;
    fragVect = fragVect / uniAtmParams[3];
    vec3 f = fragVect - uniAtmCameraPosition;
    return atmDensityDir(f, length(f));
}

vec4 atmColor(float density, vec4 color)
{
    density = clamp(density, 0.0, 1.0);
    vec3 a = mix(uniAtmColorLow.rgb, uniAtmColorHigh.rgb, pow(1.0 - density, 0.3));
    return vec4(mix(color.rgb, a, density), color.a);
}

