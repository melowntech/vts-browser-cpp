#version 330

uniform sampler2D texDepth;
uniform sampler2D texColor;

uniform vec4 uniColor;
uniform int uniUseColorTexture;

in vec2 varUvs;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = uniColor;
    if (uniUseColorTexture == 1)
        outColor *= texture(texColor, varUvs);
    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    if (gl_FragCoord.z > depthNorm)
        outColor.w *= 0.35;
}

