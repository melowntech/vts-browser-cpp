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

#ifndef GEODATA_HPP_erg546g4g14
#define GEODATA_HPP_erg546g4g14

#include <vts-browser/geodata.hpp>
#include <vts-browser/cameraDraws.hpp>
#include "renderer.hpp"

namespace vts { namespace renderer
{

class Font;

struct Subtext
{
    std::shared_ptr<Font> font;
    std::shared_ptr<Texture> texture;
    uint16 fileIndex;
    uint16 indicesStart;
    uint16 indicesCount;

    Subtext() : fileIndex(-1), indicesStart(0), indicesCount(0)
    {}
};

struct Text
{
    std::vector<float> lineVertPositions; // positions of line vertices in range -1 .. 1
    std::vector<float> lineGlyphPositions; // position of glyph's centers along the line in range -1 .. 1
    std::vector<Rect> collisionGlyphsRects;
    std::vector<vec4f> coordinates; // x, y, uv.s (+ plane index * 2), uv.t - four vec4f per glyph
    std::vector<Subtext> subtexts;
    Rect collision;
    vec2f originSize;
    float size;

    Text() : originSize(nan2().cast<float>()), size(-1)
    {}
};

struct Point
{
    vec3 worldPosition;
    vec3f worldUp;
    vec3f modelPosition;
};

class GeodataTile : public std::enable_shared_from_this<GeodataTile>
{
public:
    std::string debugId;

    GpuGeodataSpec spec;
    RenderContextImpl *renderer;
    ResourceInfo *info;
    mat4 model;
    mat4 modelInv;

    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    std::unique_ptr<UniformBuffer> uniform;

    std::vector<std::shared_ptr<Font>> fontCascade;
    std::vector<Text> texts;

    std::vector<Point> points;

    GeodataTile();
    void load(RenderContextImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp, const std::string &debugId);
    void addMemory(ResourceInfo &other);
    uint32 getTotalPoints() const;
    vec3f modelUp(const vec3f &modelPos);
    void copyPoints();
    void copyFonts();
    void loadLines();
    void loadPoints();
    void loadLabelScreens();
    void loadLabelFlats();
    void loadIcons();
    void loadTriangles();
    bool checkTextures();
};

bool regenerateJobLabelFlat(const RenderViewImpl *rv, GeodataJob &j);
void preDrawJobLabelFlat(const RenderViewImpl *rv, const GeodataJob &j,
    std::vector<vec3> &worldPos, float &scale);

} } // namespace vts renderer priv

#endif
