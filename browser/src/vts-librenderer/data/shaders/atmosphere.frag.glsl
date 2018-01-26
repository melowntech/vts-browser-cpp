
uniform sampler2D texDepthSingle;
#ifndef GL_ES
uniform sampler2DMS texDepthMulti;
#endif
uniform sampler2D texDensity;

uniform vec4 uniAtmColorLow;
uniform vec4 uniAtmColorHigh;
uniform vec4 uniParams; // projected, multisampling, atmosphere, minor-axis
uniform mat4 uniInvViewProj;
uniform vec3 uniCameraPosition; // camera position
uniform vec3 uniCameraDirection; // normalize(uniCameraPosition)
uniform vec3 uniCornerDirs[4];

in vec2 varUv;

layout(location = 0) out vec4 outColor;

const float M_PI = 3.14159265358979323846;

float sqr(float x)
{
    return x * x;
}

float linearDepth(float depthSample)
{
    vec4 p4 = uniInvViewProj * vec4(varUv * 2.0 - 1.0, depthSample * 2.0 - 1.0, 1.0);
    vec3 p = p4.xyz / p4.w;
    float depthTrue = length(p - uniCameraPosition);
    return depthTrue;
}

vec3 cameraFragmentDirection()
{
    return normalize(mix(
        mix(uniCornerDirs[0], uniCornerDirs[1], varUv.x),
        mix(uniCornerDirs[2], uniCornerDirs[3], varUv.x), varUv.y));
}

float decodeFloat(vec4 rgba)
{
    return dot(rgba, vec4(1.0, 1 / 255.0, 1 / 65025.0, 1 / 16581375.0));
}

vec3 convert(vec3 a)
{
    return a * vec3(1, 1, 1 / uniParams[3]);
}

float atmosphere(float t1)
{
    // convert from ellipsoidal into spherical space
    vec3 camPos =  convert(uniCameraPosition);
    vec3 camDir =  normalize(convert(uniCameraDirection));
    vec3 fragDir = normalize(convert(cameraFragmentDirection()));
    if (t1 < 99)
    {
        vec3 T = uniCameraPosition + t1 * cameraFragmentDirection();
        T = convert(T);
        t1 = length(T - camPos);
    }

    // ray parameters
    float l = length(camPos); // distance of camera center from world origin
    float x = dot(fragDir, -camDir) * l; // distance of point on ray (called "x"), which is closest to world origin, to camera
    float y2 = sqr(l) - sqr(x);
    float y = sqrt(y2); // distance of the ray from world origin

    float atmHeight = uniParams[2]; // atmosphere height (excluding planet radius)
    float atmRad = 1 + atmHeight; // atmosphere height including planet radius
    float atmRad2 = sqr(atmRad);

    if (y > atmRad)
        return 0; // early exit

    // fill holes in terrain
    // if the ray passes through the planet,
    //   or the camera is so far that the earths mesh is too rough ...
    if ((y < 1 && x >= 0 && t1 > 99) || l > 1.5)
    {
        float g = sqrt(1 - y2);
        t1 = x - g; // ... the t1 is moved onto the mathematical model surface
    }

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
        float fi = atan(y, t);
        vec2 uv = vec2(1 - fi / M_PI, pow(r / atmRad, 10));
        ds[i] = texture(texDensity, uv).x;
    }

    // final optical transmitance
    float density = ds[0] - ds[1];
    return 1 - pow(10, -100 * density);
}

void main()
{
    // find the actual depth (resolve multisampling, if appropiate)
    float depthSample = 0.0;
    #ifndef GL_ES
    int samples = int(uniParams[1] + 0.1);
    if (samples > 1)
    {
        for (int i = 0; i < samples; i++)
        {
            float d = texelFetch(texDepthMulti, ivec2(gl_FragCoord.xy), i).x;
            depthSample += d;
        }
        depthSample /= samples;
    }
    else
    {
    #endif
        depthSample = texelFetch(texDepthSingle, ivec2(gl_FragCoord.xy), 0).x;
    #ifndef GL_ES
    }
    #endif

    // max ray length
    float t1 = 100; // 100 times the planet radius - should be enough to consider it as infinite
    if (depthSample < 1.0)
        t1 = linearDepth(depthSample);

    // atmosphere opacity
    float atm = atmosphere(t1);
    atm = clamp(atm, 0.0, 1.0);

    // compose atmosphere color
    outColor = mix(uniAtmColorLow, uniAtmColorHigh, pow(1.0 - atm, 0.2));
    outColor.a *= atm;
}

