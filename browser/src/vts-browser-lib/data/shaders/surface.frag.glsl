#version 330

uniform sampler2D texColor;
uniform sampler2D texMask;

uniform vec4 uniColor;
uniform int uniUseMask;
uniform int uniMonochromatic;
uniform int uniFlatShading;

layout(location = 0) out vec4 outColor;

in vec3 derivativePosition;
in vec2 varUvs;

void main()
{
    if (uniFlatShading == 1)
    {
        vec3 n = normalize(cross(dFdx(derivativePosition),
                                 dFdy(derivativePosition)));
        outColor = uniColor * vec4(vec3(max(n.z * 0.8, 0) + 0.125), 1.0);
    }
    else
    {
        if (uniUseMask == 1)
        {
            if (texture(texMask, varUvs).r < 0.5)
                discard;
        }
        vec4 t = texture(texColor, varUvs);
        if (uniMonochromatic == 1)
            t = t.rrra;
        t *= uniColor;
        outColor = t;
    }
}

