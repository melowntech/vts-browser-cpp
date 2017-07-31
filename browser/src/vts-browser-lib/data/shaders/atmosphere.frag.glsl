#version 330

uniform vec4 uniColorLow;
uniform vec4 uniColorHigh;
uniform vec4 uniBodyRadiuses; // major, minor, atmosphere, rayStep
uniform vec4 uniDepths; // near. far, rayStart, rayEnd
uniform vec3 uniCameraPosition;
uniform vec3 uniCameraDirections[4];
//uniform vec4 uniViewport;
//uniform mat4 uniVpInv;

uniform sampler2D texDepth;
uniform sampler2D texColor;

in vec2 varUv;

layout(location = 0) out vec4 outColor;

void main()
{
    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    float rayEnd = uniDepths.w;
    if (depthNorm < 1)
    {
        // foreground
        float n = uniDepths.x;
        float f = uniDepths.y;
        float depthReal = (2 * n * f) / (f + n - (2 * depthNorm - 1) * (f - n));
        rayEnd = clamp(depthReal, uniDepths.z, rayEnd);
    }

    vec3 rayDir = normalize(mix(
                mix(uniCameraDirections[0], uniCameraDirections[1], varUv.x),
                mix(uniCameraDirections[2], uniCameraDirections[3], varUv.x),
                varUv.y));

    outColor = vec4(0);
    float rayPos = uniDepths.z;
    while (rayPos < rayEnd)
    {
        vec3 pos = uniCameraPosition + rayPos * rayDir;
        float alt = (length(pos) - uniBodyRadiuses.x) / uniBodyRadiuses.z;
        alt = clamp(alt, 0, 1);
        float w = mix(1, 0, alt);
        vec4 c = vec4(uniColorLow.rgb, w);
        outColor += c;
        rayPos += uniBodyRadiuses.w;
    }
    outColor.w /= (uniDepths.w - uniDepths.z) / uniBodyRadiuses.w;

    /*
    vec4 pos4 = vec4((gl_FragCoord.xy - uniViewport.xy)
                / uniViewport.zw * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    pos4 = uniVpInv * pos4;
    vec3 pos = pos4.xyz / pos4.w;

    float border = 1 - dot(normalize(pos), normalize(uniCameraPosition));
    float fog = pow(length(pos - uniCameraPosition)
                    / uniBodyRadiuses.w, 1.5);

    // todo try fog = sonmeFunction(depth);

    outColor = mix(vec4(0), uniColorLow, max(border, fog));
    */
}

