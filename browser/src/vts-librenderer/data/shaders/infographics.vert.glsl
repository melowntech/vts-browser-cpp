
layout(std140) uniform uboInfographics
{
    mat4 uniMvp;
    vec4 uniColor;
    vec4 data;
    vec4 data2;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvs;

void main()
{
    if (data[0] != 0.0)
    {
        vec4 pos = uniMvp * vec4(vec3(0.0), 1.0);

        pos.xy += ((inPosition.xy  * vec2(data[0] * data[1],-data[0]))
                + vec2(data2[2],-data2[3])) * (vec2(data[2], data[3]) * pos.w);

        //round pos
        pos.xy /= pos.w;
        pos.xy /= vec2(data[2],data[3]);
        pos.xy = round(pos.xy);
        pos.xy *= vec2(data[2],data[3]);
        pos.xy *= pos.w;

        const float texelX = 1.0 / 256.0;
        const float texelY = 1.0 / 128.0;

        varUvs = vec2((data2[0] + inUvInternal.x*16.0*data[1]-2.0) * texelX,
                        1.0-((data2[1] + inUvInternal.y * 16.0-1.0)) * texelY);

        gl_Position = pos;
    }
    else
    {
        gl_Position = uniMvp * vec4(inPosition, 1.0);
        varUvs = inUvInternal;
    }

}

