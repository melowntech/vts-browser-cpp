#version 330

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

uniform sampler2D texDepth;
uniform sampler2D texColor;

in vec2 varUv;
in vec3 varCamDir;

layout(location = 0) out vec4 outColor;

float halfPi = 3.14159265359 * 0.5;

float smootherstep(float f)
{
    return f * f * f * (f * (f * 6 - 15) + 10);
}

void main()
{
    outColor = vec4(0);

    // forward direction from camera to this fragment
    vec3 camDir = normalize(varCamDir);
    //vec4 c4f = uniInvView * vec4(varUv * 2 - 1, 1, 1);
    //vec4 c4n = uniInvView * vec4(0, 0, -1, 1);
    //vec3 camDir = normalize(c4f.xyz / c4f.w - c4n.xyz / c4n.w);

    // camera dot
    vec3 up = uniProjected == 1 ? vec3(0, 0, 1) : uniCameraPosNorm;
    float camDot = dot(camDir, up);

    // aura
    float aurDot = (camDot - uniAtmosphere.x)
            / (uniAtmosphere.y - uniAtmosphere.x);
    aurDot = clamp(aurDot, 0, 1);
    vec4 aurColor =
            vec4(mix(uniColorLow.rgb, uniColorHigh.rgb, aurDot),
                 mix(uniColorLow.a, 0, aurDot));

    // atmosphere
    float atmGrad = camDot * 2.5;
    atmGrad = clamp(atmGrad, 0, 1);
    vec4 atmColor = mix(uniColorLow, uniColorHigh, atmGrad);

    // camera altitude
    float camAlt = length(uniCameraPosition) - uniBody.x;
    camAlt = camAlt / uniBody.z;
    camAlt = clamp(camAlt, 0, 1);
    camAlt = smootherstep(camAlt);
    outColor = mix(atmColor, aurColor, camAlt);

    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    if (depthNorm < 1)
    { // foreground
        if (uniProjected == 1)
            discard;

        // reconstruct world position
        vec4 p4 = uniInvView * vec4(varUv * 2 - 1, depthNorm * 2 - 1, 1);
        vec3 p = p4.xyz / p4.w;
        float depthTrue = length(p - uniCameraPosition);

        // fog
        float fog = (depthTrue - uniPlanes.z) / (uniPlanes.w - uniPlanes.z);
        fog = clamp(fog, 0, 1);
        //fog = pow(fog, 1.2);
        outColor = mix(vec4(0), outColor, fog);
        //outColor = vec4(fog <= 0 ? 1 : 0, fog >= 1 ? 1 : 0, 0, 0.5);
    }
}

