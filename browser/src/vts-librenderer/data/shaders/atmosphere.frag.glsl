
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
    float y = sqrt(y2); // distance of the ray from world origin

    float atmHeight = uniParams[2]; // atmosphere height (excluding planet radius)
    float atmRad = 1 + atmHeight; // atmosphere height including planet radius
    float atmRad2 = sqr(atmRad);

    if (y > atmRad)
        return 0; // early exit

    // fill holes in terrain
    // if the ray passes through the planet, ...
    //if (y < 0.995 && x >= 0)
    //{
    //    float g = sqrt(1 - y2);
    //    t1 = min(t1, x - g); // ... the t1 is clamped onto the planet surface
    //}

    // distance of atmosphere boundary from "x"
    float a = sqrt(atmRad2 - y2);

    // clamp t0 and t1 to atmosphere boundaries
    // t0 and t1 is line segment that encloses the unobstructed portion of the ray and is inside atmosphere
    float t0 = max(0.0, x - a);
    t1 = min(t1, x + a);

    // texture uv coordinates
    float uDiv = 0.5 / a;
    float u0 = (t0 - x + a) * uDiv;
    float u1 = (t1 - x + a) * uDiv;
    float v = y / atmRad;
    float d0 = texture(texDensity, vec2(u0, v)).x;
    float d1 = texture(texDensity, vec2(u1, v)).x;
    float density = d0 - d1;
    return 1 - pow(10, -50 * density);


    //outColor = vec4(vec3(u0), 1);
    //return 0;

    //outColor = vec4(texture(texDensity, varUv).xxx, 1);
    //return 0;




    /*
    float sum = 0;
    float step = 0.0001;
    for (float t = t0; t < t1; t += step)
    {
        float h = sqrt(sqr(t - x) + y2);
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

