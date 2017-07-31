#version 330

uniform vec4 uniColorLow;
uniform vec4 uniColorHigh;
uniform vec4 uniBodyRadiuses; // major, minor, atmosphere
uniform vec4 uniDepths; // near. far, fogStart, fogFull
//uniform mat4 uniVpInv;
uniform vec3 uniCameraPosition;
uniform vec3 uniCameraPosNorm;
uniform vec3 uniCameraDirections[4];

uniform sampler2D texDepth;
uniform sampler2D texColor;

in vec2 varUv;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0);

    // forward direction from camera to this fragment
    vec3 camDir = normalize(mix(
                mix(uniCameraDirections[0], uniCameraDirections[1], varUv.x),
                mix(uniCameraDirections[2], uniCameraDirections[3], varUv.x),
                varUv.y));

    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    if (depthNorm < 1)
    { // foreground

        float n = uniDepths.x;
        float f = uniDepths.y;
        float depthReal = (2 * n * f) / (f + n - (2 * depthNorm - 1) * (f - n));

        // fragment position in world coordinates
        //vec4 pos4 = vec4(varUv * 2 - 1, depthNorm * 2.0 - 1.0, 1.0);
        //pos4 = uniVpInv * pos4;
        //vec3 pos2 = pos4.xyz / pos4.w;
        vec3 pos = uniCameraPosition + camDir * depthReal;

        //outColor = vec4(abs(pos2 - pos) * 0.001, 1);
        //return;

        // fog
        float fog = (depthReal - uniDepths.z) / (uniDepths.w - uniDepths.z);
        fog = clamp(fog, 0, 1);
        fog = pow(fog, 2);
        fog *= pow(1 - max(dot(camDir, -uniCameraPosNorm), 0), 5);
        outColor += mix(vec4(0), uniColorLow, fog);

        // border
        float border = 1 - dot(normalize(pos), uniCameraPosNorm);
        outColor += vec4(uniColorLow.rgb, border);
    }
    else
    { // background

        // aura

        // atmosphere
    }

    //float alt = length(uniCameraPosition) - uniBodyRadiuses.x;
    //alt = alt / uniBodyRadiuses.z;
    //alt = clamp(alt, 0, 1);
    //outColor = vec4(vec3(alt), 0.5);


    ////float d = 1 - max(dot(camDir, -normalize(uniCameraPosition)), 0);
    ////outColor = mix(uniColorLow, uniColorHigh, d);

    //vec4 pos4 = vec4(varUv * 2 - 1, depthNorm * 2.0 - 1.0, 1.0);
    //pos4 = uniVpInv * pos4;
    //vec3 pos = pos4.xyz / pos4.w;

    //float border = 1 - dot(normalize(pos), normalize(uniCameraPosition));
    //float fog = pow(length(pos - uniCameraPosition)
    //                / uniBodyRadiuses.w, 1.5);

    // todo try fog = someFunction(depth);

    //outColor = mix(vec4(0), uniColorLow, max(border, fog));
}

