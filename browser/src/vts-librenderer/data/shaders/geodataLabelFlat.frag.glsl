
layout(std140) uniform uboLabelFlat
{
    vec4 uniColor[2];
    vec4 uniOutline;
    vec4 uniPosition; // xyz
    vec4 uniCoordinates[1000];
    // even xyzw: clip space position
    // odd zw: uv (zw for compatibility with screen labels)
    // odd z: + plane index (multiplied by 2)
};

uniform sampler2D texGlyphs;

uniform int uniPass;

in vec2 varUv;
flat in int varPlane;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(varUv, 0, 1);
}

