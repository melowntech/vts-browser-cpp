
uniform sampler2D texColor;

in vec2 varUV;

out vec4 outColor;

const float lumaThreshold = 0.5;
const float mulReduce = 1.0 / 8.0;
const float minReduce = 1.0 / 128.0;
const float maxSpan = 8.0;
const vec3 toLuma = vec3(0.299, 0.587, 0.114);

void main()
{
	vec2 texelStep = 1.0 / vec2(textureSize(texColor, 0));
	vec2 uv = gl_FragCoord.xy * texelStep;
	vec3 rgbM = textureLod(texColor, uv, 0).rgb;
	if (false)
	{
		outColor = vec4(rgbM, 1.0);
		return;
	}

	vec3 rgbNW = textureLodOffset(texColor, uv, 0, ivec2(-1, 1)).rgb;
	vec3 rgbNE = textureLodOffset(texColor, uv, 0, ivec2(1, 1)).rgb;
	vec3 rgbSW = textureLodOffset(texColor, uv, 0, ivec2(-1, -1)).rgb;
	vec3 rgbSE = textureLodOffset(texColor, uv, 0, ivec2(1, -1)).rgb;
	float lumaNW = dot(rgbNW, toLuma);
	float lumaNE = dot(rgbNE, toLuma);
	float lumaSW = dot(rgbSW, toLuma);
	float lumaSE = dot(rgbSE, toLuma);
	float lumaM = dot(rgbM, toLuma);
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	if (lumaMax - lumaMin <= lumaMax * lumaThreshold)
	{
		outColor = vec4(rgbM, 1.0);
		return;
	}

	vec2 samplingDirection;
	samplingDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	samplingDirection.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * mulReduce, minReduce);
	float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);
	samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-maxSpan), vec2(maxSpan)) * texelStep;
	vec3 rgbSampleNeg = textureLod(texColor, uv + samplingDirection * (1.0/3.0 - 0.5), 0).rgb;
	vec3 rgbSamplePos = textureLod(texColor, uv + samplingDirection * (2.0/3.0 - 0.5), 0).rgb;
	vec3 rgbTwoTab = (rgbSamplePos + rgbSampleNeg) * 0.5;
	vec3 rgbSampleNegOuter = textureLod(texColor, uv + samplingDirection * (0.0/3.0 - 0.5), 0).rgb;
	vec3 rgbSamplePosOuter = textureLod(texColor, uv + samplingDirection * (3.0/3.0 - 0.5), 0).rgb;
	vec3 rgbFourTab = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoTab * 0.5;
	float lumaFourTab = dot(rgbFourTab, toLuma);
	if (lumaFourTab < lumaMin || lumaFourTab > lumaMax)
		outColor = vec4(rgbTwoTab, 1.0);
	else
		outColor = vec4(rgbFourTab, 1.0);
}

