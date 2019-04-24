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
#include "renderer.hpp"

namespace vts { namespace renderer
{

class Font;

struct Word
{
    std::shared_ptr<Font> font;
    std::shared_ptr<Texture> texture;
    uint16 fileIndex;
    uint16 coordinatesStart;
    uint16 coordinatesCount;

    Word() : fileIndex(-1), coordinatesStart(0), coordinatesCount(0)
    {}
};

struct Text
{
    std::vector<vec4f> coordinates;
    std::vector<Word> words;
    vec3 worldPosition;
    vec3f modelPosition;
    vec3f worldUp;
    vec2f rectOrigin;
    vec2f rectSize;

    Text() : worldPosition(0, 0, 0), modelPosition(0, 0, 0), worldUp(0, 0, 0),
        rectOrigin(nan2().cast<float>()),
        rectSize(nan2().cast<float>())
    {}
};

class GeodataBase : public std::enable_shared_from_this<GeodataBase>
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
    std::shared_ptr<UniformBuffer> uniform;

    vec4f outline;
    std::vector<std::shared_ptr<Font>> fontCascade;
    std::vector<Text> texts;

    GeodataBase();
    void load(RenderContextImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp, const std::string &debugId);
    void addMemory(ResourceInfo &other);
    vec3f modelUp(const vec3f &p) const;
    vec3f worldUp(vec3f &p) const;
    uint32 getTotalPoints() const;
    void prepareTextureForLinesAndPoints(Buffer &&texBuffer,
        uint32 totalPoints);
    void prepareMeshForLinesAndPoints(Buffer &&indBuffer,
        uint32 indicesCount);
    void loadLines();
    void loadPoints();
    void loadTriangles();
    void copyFonts();
    void loadPointLabels();
    void loadLineLabels();
    bool checkTextures();
};

} } // namespace vts renderer priv

#endif
