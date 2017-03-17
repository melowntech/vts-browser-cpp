#include "color.h"

namespace melown
{

const vec3f convertRgbToHsv(const vec3f &inColor)
{
    vec3f outColor;
    float minColor = inColor[0] < inColor[1] ? inColor[0] : inColor[1];
    minColor = minColor < inColor[2] ? minColor : inColor[2];
    float maxColor = inColor[0] > inColor[1] ? inColor[0] : inColor[1];
    maxColor = maxColor > inColor[2] ? maxColor : inColor[2];
    outColor[2] = maxColor;
    float delta = maxColor - minColor;
    if (delta < 0.00001)
        return outColor;
    if (maxColor > 0)
        outColor[1] = (delta / maxColor);
    else
        return outColor;
    if (inColor[0] >= maxColor)
        outColor[0] = (inColor[1] - inColor[2]) / delta;
    else
        if (inColor[1] >= maxColor)
            outColor[0] = 2 + (inColor[2] - inColor[0]) / delta;
        else
            outColor[0] = 4 + (inColor[0] - inColor[1]) / delta;
    outColor[0] *= 60.f / 360.f;
    if (outColor[0] < 0)
        outColor[0] += 1;
    return outColor;
}

const vec3f convertHsvToRgb(const vec3f &inColor)
{
    vec3f outColor;
    if (inColor[1] <= 0)
    {
        outColor[0] = inColor[2];
        outColor[1] = inColor[2];
        outColor[2] = inColor[2];
        return outColor;
    }
    float hh = inColor[0];
    if (hh >= 1)
        hh = 0;
    hh /= 60.f / 360.f;
    uint32 i = (uint32)hh;
    float ff = hh - i;
    float p = inColor[2] * (1 - inColor[1]);
    float q = inColor[2] * (1 - (inColor[1] * ff));
    float t = inColor[2] * (1 - (inColor[1] * (1 - ff)));
    switch (i)
    {
    case 0:
        outColor[0] = inColor[2];
        outColor[1] = t;
        outColor[2] = p;
        break;
    case 1:
        outColor[0] = q;
        outColor[1] = inColor[2];
        outColor[2] = p;
        break;
    case 2:
        outColor[0] = p;
        outColor[1] = inColor[2];
        outColor[2] = t;
        break;
    case 3:
        outColor[0] = p;
        outColor[1] = q;
        outColor[2] = inColor[2];
        break;
    case 4:
        outColor[0] = t;
        outColor[1] = p;
        outColor[2] = inColor[2];
        break;
    case 5:
    default:
        outColor[0] = inColor[2];
        outColor[1] = p;
        outColor[2] = q;
        break;
    }
    return outColor;
}

} // namespace melown

