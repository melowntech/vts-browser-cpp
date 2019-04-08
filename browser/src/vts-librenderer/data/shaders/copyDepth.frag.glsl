
uniform sampler2D texDepth;

out vec4 outDepth;

vec4 packFloatToVec4(float value)
{
    vec4 bitSh = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
    vec4 bitMsk = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
    vec4 res = fract(value * bitSh);
    res -= res.xxyz * bitMsk;
    return res;
}

void main()
{
    outDepth = packFloatToVec4(texelFetch(texDepth, ivec2(gl_FragCoord) * 3, 0).r);
}

