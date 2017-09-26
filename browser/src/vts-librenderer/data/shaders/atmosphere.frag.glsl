
uniform sampler2D texDepthSingle;
#ifndef GL_ES
uniform sampler2DMS texDepthMulti;
#endif

uniform vec4 uniColorLow;
uniform vec4 uniColorHigh;
uniform vec4 uniBody; // major, minor, atmosphere thickness
uniform vec4 uniPlanes; // near. far, fogStart, fogFull
uniform vec4 uniAtmosphere; // minAngle, maxAngle, horizonAngle
uniform vec3 uniCameraPosition;
uniform vec3 uniCameraPosNorm;
uniform int uniProjected;
          // uniCameraDirections
uniform mat4 uniInvView;
uniform int uniMultiSamples;

in vec2 varUv;
in vec3 varCamDir;

layout(location = 0) out vec4 outColor;

float halfPi = 3.14159265359 * 0.5;

float smootherstep(float f)
{
    return f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
}

void main()
{
    outColor = vec4(0.0);

    // forward direction from camera to this fragment
    vec3 camDir = normalize(varCamDir);

    // camera dot
    vec3 up = uniProjected == 1 ? vec3(0.0, 0.0, 1.0) : uniCameraPosNorm;
    float camDot = dot(camDir, up);

    // aura
    float aurDot = (camDot - uniAtmosphere.x)
            / (uniAtmosphere.y - uniAtmosphere.x);
    aurDot = clamp(aurDot, 0.0, 1.0);
    vec4 aurColor =
            vec4(mix(uniColorLow.rgb, uniColorHigh.rgb, aurDot),
                 mix(uniColorLow.a, 0.0, aurDot));

    // atmosphere
    float atmGrad = camDot * 2.5;
    atmGrad = clamp(atmGrad, 0.0, 1.0);
    vec4 atmColor = mix(uniColorLow, uniColorHigh, atmGrad);

    // camera altitude
    float camAlt = length(uniCameraPosition) - uniBody.x;
    camAlt = camAlt / uniBody.z;
    camAlt = clamp(camAlt, 0.0, 1.0);
    camAlt = smootherstep(camAlt);
    outColor = mix(atmColor, aurColor, camAlt);

    // find the depth and antialiasing ratio
    float depthNorm = 0.0;
    float antialiasing = 0.0;
    #ifndef GL_ES
    if (uniMultiSamples > 1.0)
    {
        for (int i = 0; i < uniMultiSamples; i++)
        {
            float d = texelFetch(texDepthMulti, ivec2(gl_FragCoord.xy), i).x;
            depthNorm += d;
            antialiasing += d < 1.0 ? 1.0 : 0.0;
        }
        depthNorm /= uniMultiSamples;
        antialiasing /= uniMultiSamples;
    }
    else
    {
    #endif
        depthNorm = texelFetch(texDepthSingle, ivec2(gl_FragCoord.xy), 0).x;
        antialiasing = depthNorm < 1.0 ? 1.0 : 0.0;
    #ifndef GL_ES
    }
    #endif

    // mix fog, atmosphere and surface
    if (depthNorm < 1.0)
    {
        // foreground
        if (uniProjected == 1)
            discard;

        // reconstruct world position
        vec4 p4 = uniInvView * vec4(varUv * 2.0 - 1.0, depthNorm * 2.0 - 1.0, 1.0);
        vec3 p = p4.xyz / p4.w;
        float depthTrue = length(p - uniCameraPosition);

        // fog
        float fog = (depthTrue - uniPlanes.z) / (uniPlanes.w - uniPlanes.z);
        fog = clamp(fog, 0.0, 1.0);
        vec4 fogColor = mix(vec4(0.0), outColor, fog);

        // antialiasing
        outColor = mix(outColor, fogColor, antialiasing);
    }
}

