#version 330

uniform vec4 uniColorLow;
uniform vec4 uniColorHigh;
uniform vec3 uniBodyRadiuses; // major, minor, atmosphere thickness
uniform vec3 uniCorners[4];

in vec2 varUv;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 corner = mix(
			mix(uniCorners[0], uniCorners[1], varUv.x),
			mix(uniCorners[2], uniCorners[3], varUv.x),
			varUv.y);
	float altitude = (length(corner) - uniBodyRadiuses[0]) / uniBodyRadiuses[2];
	altitude = clamp(altitude, 0, 1);
	outColor = mix(uniColorLow, uniColorHigh, altitude);
}

