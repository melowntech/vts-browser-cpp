
uniform sampler2D texDepth;
uniform sampler2D texColor;

layout(std140) uniform uboInfographics
{
    mat4 uniMvp;
    vec4 uniColor;
    ivec4 uniUseColorTexture;
};

in vec2 varUvs;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = uniColor;
    if (uniUseColorTexture.x == 1)
        outColor *= texture(texColor, varUvs);
    float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
    if (gl_FragCoord.z > depthNorm)
        outColor.a *= 0.1;
}

