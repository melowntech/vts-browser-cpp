
uniform sampler2D texDepthSingle;
#ifndef GL_ES
uniform sampler2DMS texDepthMulti;
#endif
uniform sampler2D texDensity;

uniform vec4 uniAtmColorLow;
uniform vec4 uniAtmColorHigh;
uniform ivec4 uniParamsI; // projected, multisampling
uniform vec4 uniParamsF; // atmosphere thickness, atmosphere horizontal exponent, minor-axis
uniform vec4 uniNearFar;
uniform vec3 uniCameraPosition; // camera position
uniform vec3 uniCornerDirs[4];

in vec3 varCfd; // camera fragment direction
flat in vec3 varCvd; // camera view direction

layout(location = 0) out vec4 outColor;

const float M_PI = 3.14159265358979323846;

float sqr(float x)
{
    return x * x;
}

float linearDepth(float depthSample)
{
    float z = uniNearFar[3] / (depthSample - uniNearFar[2]);
    return z / dot(normalize(varCfd), varCvd);
}

float decodeFloat(vec4 rgba)
{
    return dot(rgba, vec4(1.0, 1.0 / 256.0, 1.0 / (256.0*256.0), 1.0 / (256.0*256.0*256.0)));
}

float sampleDensity(vec2 uv)
{
    // since some color channels of the density texture are not continuous
    //   it is important to first decode the float from rgba and only after
    //   that to filter the texture
    ivec2 res = textureSize(texDensity, 0);
    vec2 uvp = uv * vec2(res - 1);
    ivec2 iuv = ivec2(uvp); // upper-left texel fetch coordinates
    vec4 s;
    s.x = decodeFloat(texelFetchOffset(texDensity, iuv, 0, ivec2(0,0)));
    s.y = decodeFloat(texelFetchOffset(texDensity, iuv, 0, ivec2(1,0)));
    s.z = decodeFloat(texelFetchOffset(texDensity, iuv, 0, ivec2(0,1)));
    s.w = decodeFloat(texelFetchOffset(texDensity, iuv, 0, ivec2(1,1)));
    vec2 f = fract(uvp); // interpolation factors
    vec2 a = mix(s.xz, s.yw, f.x);
    float b = mix(a.x, a.y, f.y);
    return b * 5.0;
}

float atmosphereDensity(float depthSample)
{
    // max ray length
    float t1 = 100.0; // 100 times the planet radius - should be enough to consider it as infinite
    if (depthSample < 1.0)
        t1 = linearDepth(depthSample);

    // convert from ellipsoidal into spherical space
    vec3 cfd = normalize(varCfd);
    vec3 ellipseToSphere = vec3(1.0, 1.0, 1.0 / uniParamsF[2]);
    vec3 camPos =  uniCameraPosition * ellipseToSphere;
    vec3 camDir =  normalize(camPos);
    vec3 fragDir = normalize(cfd * ellipseToSphere);
    if (t1 < 99.0)
    {
        vec3 T = uniCameraPosition + t1 * cfd;
        T *= ellipseToSphere;
        t1 = length(T - camPos);
    }

    // ray parameters
    float l = length(camPos); // distance of camera center from world origin
    float x = dot(fragDir, -camDir) * l; // distance of point on ray (called "x"), which is closest to world origin, to camera
    float y2 = sqr(l) - sqr(x);
    float y = sqrt(y2); // distance of the ray from world origin

    float atmHeight = uniParamsF[0]; // atmosphere height (excluding planet radius)
    float atmRad = 1.0 + atmHeight; // atmosphere height including planet radius
    float atmRad2 = sqr(atmRad);

    if (y > atmRad)
        return 0.0; // early exit

    float t1e = x - sqrt(1.0 - y2); // t1 at ellipse
    
    // fill holes in terrain if the ray passes through the planet
    if (y < 0.998 && x >= 0.0 && t1 > 99.0)
        t1 = t1e;

    // approximate the planet by the ellipsoid if the the mesh is too rough
    if (y <= 1.0)
        t1 = mix(t1, t1e, clamp((l - 1.05) / 0.05, 0.0, 1.0));

    // to improve accuracy, swap direction of the ray to point out of the terrain
    // bc. later subtracting small numbers is more precise than subtracting large numbers
    bool swapDirection = false;
    if (t1 < 99.0 && x >= 0.0)
        swapDirection = true;

    // distance of atmosphere boundary from "x"
    float a = sqrt(atmRad2 - y2);

    // clamp t0 and t1 to atmosphere boundaries
    // t0 and t1 is line segment that encloses the unobstructed portion of the ray and is inside atmosphere
    float t0 = max(0.0, x - a);
    t1 = min(t1, x + a);

    // sample the density texture
    float ts[2];
    ts[0] = t0;
    ts[1] = t1;
    float ds[2];
    for (int i = 0; i < 2; i++)
    {
        float t = x - ts[i];
        float r = sqrt(sqr(t) + y2);
        vec2 uv = vec2(0.5 - 0.5 * t / r, 0.5 + 0.5 * (r - 1.0) / atmHeight);
        if (swapDirection)
            uv.x = 1.0 - uv.x;
        ds[i] = sampleDensity(uv);
    }

    // final optical transmittance
    float density = ds[0] - ds[1];
    if (swapDirection)
        density *= -1.0;
    return 1.0 - pow(10.0, -uniParamsF[1] * density);

    // density texture visualization
    //outColor = vec4(vec3(decodeFloat(texture(texDensity, varUv)) * 5), 1);
    
    /*
    // use numeric integration instead of the precomputed texture density
    float sum = 0.0;
    float step = 0.0001;
    for (float t = t0; t < t1; t += step)
    {
        float h = sqrt(sqr(t - x) + y2);
        h = (clamp(h, 1.0, atmRad) - 1.0) / atmHeight;
        float a = exp(-12.0 * h);
        sum += a;
    }
    sum *= step;
    return 1 - pow(10.0, -uniParamsF[1] * sum);
    */
}

void main()
{
    float atm = 0.0;
    #ifndef GL_ES
    if (uniParamsI[1] > 1)
    {
        for (int i = 0; i < uniParamsI[1]; i++)
        {
            float d = texelFetch(texDepthMulti, ivec2(gl_FragCoord.xy), i).x;
            atm += atmosphereDensity(d);
        }
        atm /= uniParamsI[1];
    }
    else
    {
    #endif
        float d = texelFetch(texDepthSingle, ivec2(gl_FragCoord.xy), 0).x;
        atm = atmosphereDensity(d);
    #ifndef GL_ES
    }
    #endif

    // compose atmosphere color
    atm = clamp(atm, 0.0, 1.0);
    outColor = mix(uniAtmColorLow, uniAtmColorHigh, pow(1.0 - atm, 0.3));
    outColor.a *= atm;
}

