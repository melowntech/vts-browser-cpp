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

#ifndef GEODATA_HPP_d5f4g65de
#define GEODATA_HPP_d5f4g65de

#include <array>
#include <vector>
#include <string>
#include <memory>

#include "foundation.hpp"

namespace vts
{

// information about geodata passed to loadGeodata callback
class VTS_API GpuGeodataSpec
{
public:
    enum class Type : uint8
    {
        Invalid,
        Triangles,
        LineFlat,
        PointFlat,
        IconFlat,
        LabelFlat,
        LineScreen,
        PointScreen,
        IconScreen,
        LabelScreen,
    };

    enum class Units : uint8
    {
        Invalid,
        Pixels,
        Meters,
        Ratio,
    };

    enum class TextAlign : uint8
    {
        Invalid,
        Left,
        Right,
        Center,
    };

    enum class Origin : uint8
    {
        Invalid,
        TopLeft,
        TopRight,
        TopCenter,
        CenterLeft,
        CenterRight,
        CenterCenter,
        BottomLeft,
        BottomRight,
        BottomCenter,
    };

    enum class PolygonStyle : uint8
    {
        Invalid,
        Solid,
        FlatShade,
    };

    struct VTS_API Point
    {
        float color[4];
        float radius;
        Units units;
    };

    struct VTS_API Line
    {
        float color[4];
        float width;
        Units units;
    };

    struct VTS_API Icon
    {
        float color[4];
        float offset[2]; // x, y (pixels)
        float margin[2]; // horizontal, vertical (pixels)
        float scale;
        Origin origin;
    };

    struct VTS_API LabelFlat
    {
        float color[4];
        float color2[4];
        float size;
        float offset;
    };

    struct VTS_API LabelScreen
    {
        float outline[4];
        float color[4];
        float color2[4];
        float offset[2]; // x, y (pixels)
        float margin[2]; // horizontal, vertical (pixels)
        float size; // vertical (pixels)
        float width; // line wrap (pixels)
        Origin origin;
        TextAlign textAlign;
    };

    struct VTS_API Triangles
    {
        float color[4];
        PolygonStyle style;
        bool useStencil;
    };

    union VTS_API UnionData
    {
        Point point;
        Line line;
        LabelFlat labelFlat;
        LabelScreen labelScreen;
        Triangles triangles;
        UnionData();
    };

    struct VTS_API Stick
    {
        float color[4];
        float heightMax;
        float heightThreshold;
        float width;
        float offset;
    };

    struct VTS_API CommonData
    {
        Stick stick;
        Icon icon;
        float visibilities[4]; // feature distance (meters), (altered) view-min, view-max, culling degrees
        float zBufferOffset[3]; // do NOT ask for units here
        float tileVisibility[2]; // view extent range in which the tile is visible
        float hysteresisDuration[2]; // seconds to appear, seconds to disappear
        float importanceDistanceFactor;
        float featuresLimitPerPixelSquared;
        float depthVisibilityThreshold; // meters
        sint32 zIndex;
        bool preventOverlap;
        CommonData();
    };

    GpuGeodataSpec();

    // positions
    std::vector<std::vector<std::array<float, 3>>> positions;

    // properties per item
    std::vector<std::array<float, 6>> iconCoords; // uv x1, uv y1, uv x2, uv y2, pixels width, pixels height
    std::vector<std::string> texts;
    std::vector<std::string> hysteresisIds;
    std::vector<float> importances;

    // global properties
    std::vector<std::shared_ptr<void>> fontCascade;
    std::shared_ptr<void> bitmap;
    double model[16];
    UnionData unionData;
    CommonData commonData;
    Type type;
};

} // namespace vts

#endif
