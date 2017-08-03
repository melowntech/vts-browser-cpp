#version 330

uniform vec4 uniColorLow;
uniform vec4 uniColorHigh;
uniform vec4 uniRadiuses; // major, minor, atmosphere thickness
uniform vec4 uniDepths; // near. far
uniform vec4 uniFog; // start, full
uniform vec4 uniAura; // minAngleDot, maxAngleDot
uniform vec3 uniCameraPosition;
uniform vec3 uniCameraPosNorm;
          // uniCameraDirections

uniform sampler2D texDepth;
uniform sampler2D texColor;

in vec2 varUv;
in vec3 varCamDir;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0);

    // forward direction from camera to this fragment
    vec3 camDir = normalize(varCamDir);

    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    if (depthNorm < 1)
    { // foreground
        // linearized depth
        float n = uniDepths.x;
        float f = uniDepths.y;
        float depthReal = (2 * n * f) / (f + n - (2 * depthNorm - 1) * (f - n));

        // fragment position in world coordinates
        vec3 pos = uniCameraPosition + camDir * depthReal;

        // fog
        float fog = (depthReal - uniFog.x) / (uniFog.y - uniFog.x);
        fog = clamp(fog, 0, 1);
        fog = pow(fog, 3);
        fog *= pow(1 - max(dot(camDir, -uniCameraPosNorm), 0), 10);
        outColor += mix(vec4(0), uniColorLow, fog);

        // border
        float border = 1 - max(dot(normalize(pos), uniCameraPosNorm), 0);
        outColor += mix(vec4(0), uniColorLow, border);
    }
    else
    { // background
        // camera dot
        float camDot = dot(camDir, uniCameraPosNorm);

        // aura
        float aurDot = (camDot - uniAura.x) / (uniAura.y - uniAura.x);
        aurDot = clamp(aurDot, 0, 1);
        vec4 aurColor = vec4(mix(uniColorLow.rgb, uniColorHigh.rgb, aurDot),
                             mix(uniColorLow.a, 0, aurDot));

        // atmosphere
        float atmGrad = camDot * 2.5;
        atmGrad = clamp(atmGrad, 0, 1);
        vec4 atmColor = mix(uniColorLow, uniColorHigh, atmGrad);

        // camera altitude
        float camAlt = length(uniCameraPosition) - uniRadiuses.x;
        camAlt = camAlt / uniRadiuses.z;
        camAlt = clamp(camAlt, 0, 1);
        camAlt = smoothstep(0, 1, camAlt);
        outColor = mix(atmColor, aurColor, camAlt);
    }
}

