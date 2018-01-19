
uniform sampler2D texDepthSingle;
#ifndef GL_ES
uniform sampler2DMS texDepthMulti;
#endif
uniform sampler2D texDensity;

uniform vec4 uniAtmColorLow;
uniform vec4 uniAtmColorHigh;
uniform vec4 uniParams; // projected, multisampling, atmosphere
uniform mat4 uniInvViewProj;
uniform vec3 uniCameraPosition; // camera position
uniform vec3 uniCameraDirection; // normalize(uniCameraPosition)
uniform vec3 uniCornerDirs[4];

in vec2 varUv;

layout(location = 0) out vec4 outColor;

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


    vec4 f4 = vec4(varUv * 2 - 1, 0, 1);
    f4 = uniInvViewProj * f4;
    vec3 f = f4.xyz / f4.w;
    vec3 dir2 = normalize(f - uniCameraPosition);

    //return abs(dir - dir2) * 10;

    /*
    vec4 n4 = vec4(varUv * 2 - 1, 0, 1);
    n4 = uniInvViewProj * n4;
    vec4 f4 = n4 + uniInvViewProj[2];
    vec3 n = n4.xyz / n4.w;
    vec3 f = f4.xyz / f4.w;
    return normalize(f - n);
    */
}

float decodeFloat(vec4 rgba)
{
    return dot(rgba, vec4(1.0, 1 / 255.0, 1 / 65025.0, 1 / 16581375.0));
}

float atmosphere(float t1)
{
    // ray parameters
    float l = length(uniCameraPosition); // distance of camera center from world origin
    float x = dot(cameraFragmentDirection(), -uniCameraDirection) * l; // distance of point on ray (called "x"), which is closest to world origin, to camera
    float y2 = sqr(l) - sqr(x);
    float y = sqrt(y2); // distance of ray from world origin

    // fill holes in terrain
    float g = 0;
    // if the ray passes through the planet, ...
    if (y < 0.995 && x >= 0)
    {
        g = sqrt(1 - y2);
        t1 = min(t1, x - g); // ... the t1 is clamped onto the planet surface
    }

    float atmHeight = uniParams[2]; // atmosphere height (excluding planet radius)
    float atmRad = 1 + atmHeight; // atmosphere height including planet radius

    if (y > atmRad)
        return 0; // early exit

    float a = sqrt(sqr(atmRad) - y2); // distance of atmosphere boundary from "x"
    float t0 = max(0.0, x - a) - x;
    t1 = min(t1, x + a) - x;
    // t0 and t1 are points on the ray (relative to "x"), that enclose the unobstructed portion of the ray and are inside atmosphere

    // map parameters to uv and sample the atmosphere density texture
    float v = y / atmRad;
    float u0 = abs(t0) / a;
    float u1 = abs(t1) / a;
    //float d0 = decodeFloat(texture(texDensity, vec2(u0, v)));
    //float d1 = decodeFloat(texture(texDensity, vec2(u1, v)));
    float d0 = texture(texDensity, vec2(u0, v)).x;
    float d1 = texture(texDensity, vec2(u1, v)).x;

    // compute total atmosphere density along the ray from t0 to t1
    float density;
    if (t0 * t1 < 0) // resolve the density samples (whether the ray crosses a symetry axis)
        density = d0 + d1;
    else
        density = abs(d0 - d1);

    // compute optical opacity from the density
    return 1 - pow(10, -50 * density);




    //outColor = vec4(vec3(decodeFloat(texture(texDensity, varUv))), 1);
    //outColor = vec4(texture(texDensity, varUv).xxx, 1);
    //return 0;




    /*
    float sum = 0;
    float step = 0.0001;
    for (float t = t0; t < t1; t += step)
    {
        float h = sqrt(sqr(t) + y2);
        h = (clamp(h, 1, atmRad) - 1) / atmHeight;
        float a = exp(-12 * h);
        sum += a;
    }
    sum *= step;
    return 1 - pow(10, -50 * sum);
    */
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
    outColor = mix(uniAtmColorLow, uniAtmColorHigh, 1.0 - atm);
    outColor.a *= atm;
}

