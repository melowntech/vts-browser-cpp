
uniform sampler2D texDepth;
uniform ivec2 uniTexPos;

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
    outDepth = packFloatToVec4(texelFetch(texDepth, uniTexPos, 0).r);
}

