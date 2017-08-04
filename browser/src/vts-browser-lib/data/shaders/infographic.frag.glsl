#version 330

uniform vec4 uniColor;
uniform int uniUseColorTexture;

uniform sampler2D texDepth;
uniform sampler2D texColor;

layout(location = 0) out vec4 outColor;

in vec2 varUvs;

void main()
{
    outColor = uniColor;
    if (uniUseColorTexture == 1)
        outColor *= texture(texColor, varUvs);
    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    if (gl_FragCoord.z > depthNorm)
        outColor.w *= 0.25;
}

