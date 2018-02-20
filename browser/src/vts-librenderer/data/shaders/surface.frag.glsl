
uniform sampler2D texColor;
uniform sampler2D texMask;

uniform vec4 uniColor;
uniform vec4 uniUvClip;
uniform ivec4 uniFlags; // mask, monochromatic, flat shading, uv source

in vec2 varUvInternal;
in vec2 varUvExternal;
in vec3 varViewPosition;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 uv = uniFlags.w > 0 ? varUvExternal : varUvInternal;
    vec4 color = texture(texColor, uv);

    // mask
    if (uniFlags.x > 0)
    {
        if (texture(texMask, uv).r < 0.5)
            discard;
    }

    // uv clipping
    if (varUvExternal.x < uniUvClip.x
            || varUvExternal.y < uniUvClip.y
            || varUvExternal.x > uniUvClip.z
            || varUvExternal.y > uniUvClip.w)
        discard;

    // base color
    if (uniFlags.z > 0)
    {
        // flat shading
        vec3 n = normalize(cross(dFdx(varViewPosition),
                                 dFdy(varViewPosition)));
        outColor = vec4(vec3(max(n.z * 0.8, 0.0) + 0.125), 1.0);
    }
    else
    {
        // texture color
        if (uniFlags.y > 0)
            color = color.rrra; // monochromatic texture
        outColor = color;
    }
    outColor *= uniColor;

    // atmosphere
    float atmosphere = atmDensity(varViewPosition);
    outColor = atmColor(atmosphere, outColor);
}

