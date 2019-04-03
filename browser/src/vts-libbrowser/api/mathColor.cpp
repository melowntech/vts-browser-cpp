/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "../include/vts-browser/math.hpp"

namespace vts
{

vec3f convertRgbToHsv(const vec3f &inColor)
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

vec3f convertHsvToRgb(const vec3f &inColor)
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

vec3f convertToRainbowColor(float inValue)
{
    float value = 4.0f * (1.0f - inValue);
    value = clamp(value, 0, 4);
    int band = int(value);
    value -= band;
    vec3f result;
    switch (band)
    {
    case 0:
        result[0] = 1;
        result[1] = value;
        result[2] = 0;
        break;
    case 1:
        result[0] = 1.0f - value;
        result[1] = 1;
        result[2] = 0;
        break;
    case 2:
        result[0] = 0;
        result[1] = 1;
        result[2] = value;
        break;
    case 3:
        result[0] = 0;
        result[1] = 1.0f - value;
        result[2] = 1;
        break;
    default:
        result[0] = value;
        result[1] = 0;
        result[2] = 1;
        break;
    }
    return result;
}

} // namespace vts

