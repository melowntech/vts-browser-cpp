#version 330

uniform vec4 uniColorLow;
uniform vec4 uniColorHigh;
uniform vec4 uniBody; // major, minor, atmosphere thickness
uniform vec4 uniPlanes; // near. far
uniform vec4 uniAtmosphere; // minAngle, maxAngle, horizonAngle, fogDistance
uniform vec3 uniCameraPosition;
uniform vec3 uniCameraPosNorm;
          // uniCameraDirections

uniform sampler2D texDepth;
uniform sampler2D texColor;

in vec2 varUv;
in vec3 varCamDir;

layout(location = 0) out vec4 outColor;

float halfPi = 3.14159265359 * 0.5;

void main()
{
    outColor = vec4(0);

    // forward direction from camera to this fragment
    vec3 camDir = normalize(varCamDir);

    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    if (depthNorm < 1)
    { // foreground
        // linearized depth
        float n = uniPlanes.x;
        float f = uniPlanes.y;
        float depthReal = (2 * n * f) / (f + n - (2 * depthNorm - 1) * (f - n));

        // fragment position in world coordinates
        vec3 pos = uniCameraPosition + camDir * depthReal;

        // fog
        float fog = depthReal / uniAtmosphere.w;
        fog = clamp(fog, 0, 1);
        fog = pow(fog, 3);
        fog *= pow(1 - max(dot(camDir, -uniCameraPosNorm), 0), 10);
        outColor += mix(vec4(0), uniColorLow, fog);

        // border
        float border = dot(normalize(pos), uniCameraPosNorm);
        border = (border - uniAtmosphere.z + 2e-3)
                / (1 - uniAtmosphere.z + 2e-3);
        outColor += mix(uniColorLow,  vec4(0), border);
    }
    else
    { // background
        // camera dot
        float camDot = dot(camDir, uniCameraPosNorm);

        // aura
        float aurDot = (camDot - uniAtmosphere.x)
                / (uniAtmosphere.y - uniAtmosphere.x);
        aurDot = clamp(aurDot, 0, 1);
        vec4 aurColor = vec4(mix(uniColorLow.rgb, uniColorHigh.rgb, aurDot),
                             mix(uniColorLow.a, 0, aurDot));

        // atmosphere
        float atmGrad = camDot * 2.5;
        atmGrad = clamp(atmGrad, 0, 1);
        vec4 atmColor = mix(uniColorLow, uniColorHigh, atmGrad);

        // camera altitude
        float camAlt = length(uniCameraPosition) - uniBody.x;
        camAlt = camAlt / uniBody.z;
        camAlt = clamp(camAlt, 0, 1);
        camAlt = smoothstep(0, 1, camAlt);
        outColor = mix(atmColor, aurColor, camAlt);
    }
}

