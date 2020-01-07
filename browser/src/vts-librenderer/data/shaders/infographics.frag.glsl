
uniform sampler2D texDepth;
uniform sampler2D texColor;

layout(std140) uniform uboInfographics
{
    mat4 uniMvp;
    vec4 uniColor;
    vec4 uniFlags; // type, useTexture, useDepth
    vec4 data;
    vec4 data2;
};

in vec2 varUvs;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = uniColor;
    if (uniFlags[1] > 0.5)
        outColor *= texture(texColor, varUvs);

    if (uniFlags[2] > 0.5)
    {
        float depthNorm = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x;
        if (gl_FragCoord.z > depthNorm)
            outColor.a *= 0.1;
    }
}

