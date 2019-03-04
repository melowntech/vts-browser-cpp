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

#include <map>
#include <list>

#include "renderer.hpp"
#include "font.hpp"

#include <vts-browser/resources.hpp>
#include <vts-browser/cameraDraws.hpp>

extern "C"
{
#include <SheenBidi.h>
}

namespace vts { namespace renderer
{

namespace
{

class GeodataBase
{
public:
    std::string debugId;

    GeodataBase() : renderer(nullptr), info(nullptr)
    {}
    virtual ~GeodataBase()
    {}

    GpuGeodataSpec spec;
    RendererImpl *renderer;
    ResourceInfo *info;
    mat4 model;
    mat4 modelInv;

    virtual void load(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp, const std::string &debugId) = 0;

    void loadInit(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp, const std::string &debugId)
    {
        this->debugId = debugId;

        this->spec = std::move(specp);
        this->info = &info;
        this->renderer = renderer;

        model = rawToMat4(spec.model);
        modelInv = model.inverse();

        {
            // culling (degrees to dot)
            float &c = this->spec.commonData.visibilities[3];
            if (c == c)
                c = std::cos(c * M_PI / 180.0);
        }
    }

    void loadFinish()
    {
        // free some memory
        std::vector<std::vector<std::array<float, 3>>>()
            .swap(spec.positions);
        std::vector<std::vector<std::array<float, 2>>>()
            .swap(spec.uvs);
        std::vector<std::shared_ptr<void>>().swap(spec.bitmaps);
        std::vector<std::string>().swap(spec.texts);
        std::vector<std::shared_ptr<void>>().swap(spec.fontCascade);

        info->ramMemoryCost += sizeof(spec);
        info = nullptr;
        renderer = nullptr;

        CHECK_GL("load geodata");
    }

    void addMemory(ResourceInfo &other)
    {
        info->ramMemoryCost += other.ramMemoryCost;
        info->gpuMemoryCost += other.gpuMemoryCost;
    }

    vec3f modelUp(const vec3f &p) const
    {
        vec3 dl = p.cast<double>();
        vec3 dw = vec4to3(vec4(model * vec3to4(dl, 1)));
        vec3 upw = normalize(dw);
        vec3 tw = dw + upw;
        vec3 tl = vec4to3(vec4(modelInv * vec3to4(tw, 1)));
        return (tl - dl).cast<float>();
    }

    vec3f worldUp(vec3f &p) const
    {
        vec3 dl = p.cast<double>();
        vec3 dw = vec4to3(vec4(model * vec3to4(dl, 1)));
        vec3 upw = normalize(dw);
        return upw.cast<float>();
    }

    uint32 getTotalPoints() const
    {
        uint32 totalPoints = 0;
        uint32 linesCount = spec.positions.size();
        for (uint32 li = 0; li < linesCount; li++)
            totalPoints += spec.positions[li].size();
        return totalPoints;
    }
};

class GeodataGeometry : public GeodataBase
{
public:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<UniformBuffer> uniform;

    void load(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp, const std::string &debugId) override
    {
        loadInit(renderer, info, specp, debugId);

        switch (spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineFlat:
            loadLine();
            break;
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointFlat:
            loadPoint();
            break;
        case GpuGeodataSpec::Type::Triangles:
            // todo
            break;
        default:
            throw std::invalid_argument("invalid geodata type");
        }

        loadFinish();
    }

    void prepareTextureForLinesAndPoints(Buffer &&texBuffer,
        uint32 totalPoints)
    {
        GpuTextureSpec tex;
        tex.buffer = std::move(texBuffer);
        tex.width = totalPoints;
        tex.height = 2;
        tex.components = 3;
        tex.internalFormat = GL_RGB32F;
        tex.type = GpuTypeEnum::Float;
        tex.filterMode = GpuTextureSpec::FilterMode::Nearest;
        tex.wrapMode = GpuTextureSpec::WrapMode::ClampToEdge;
        ResourceInfo ri;
        renderer->rendererApi->loadTexture(ri, tex, debugId);
        this->texture = std::static_pointer_cast<Texture>(ri.userData);
        addMemory(ri);
    }

    void prepareMeshForLinesAndPoints(Buffer &&indBuffer,
        uint32 indicesCount)
    {
        GpuMeshSpec msh;
        msh.faceMode = GpuMeshSpec::FaceMode::Triangles;
        msh.indices = std::move(indBuffer);
        msh.indicesCount = indicesCount;
        ResourceInfo ri;
        renderer->rendererApi->loadMesh(ri, msh, debugId);
        this->mesh = std::static_pointer_cast<Mesh>(ri.userData);
        addMemory(ri);
    }

    void loadLine()
    {
        uint32 totalPoints = getTotalPoints(); // example: 7
        uint32 linesCount = spec.positions.size(); // 2
        uint32 segmentsCount = totalPoints - linesCount; // 5
        uint32 jointsCount = segmentsCount - linesCount; // 3
        uint32 trianglesCount = (segmentsCount + jointsCount) * 2; // 16
        uint32 indicesCount = trianglesCount * 3; // 48
        // point index = (vertex index / 4 + vertex index % 2)
        // corner = vertex index % 4

        Buffer texBuffer;
        Buffer indBuffer;

        // prepare texture buffer and mesh indices
        {
            texBuffer.resize(totalPoints * sizeof(vec3f) * 2);
            vec3f *bufPos = (vec3f*)texBuffer.data();
            vec3f *bufUps = (vec3f*)texBuffer.data() + totalPoints;
            vec3f *texBufHalf = bufUps;
            (void)texBufHalf;

            indBuffer.resize(indicesCount * sizeof(uint16));
            uint16 *bufInd = (uint16*)indBuffer.data();
            uint16 current = 0;

            for (uint32 li = 0; li < linesCount; li++)
            {
                const std::vector<std::array<float, 3>> &points
                    = spec.positions[li];
                uint32 pointsCount = points.size();
                for (uint32 pi = 0; pi < pointsCount; pi++)
                {
                    vec3f p = rawToVec3(points[pi].data());
                    vec3f u = modelUp(p);
                    *bufPos++ = p;
                    *bufUps++ = u;
                    if (pi > 1)
                    { // add joint
                        *bufInd++ = current + 1 - 4;
                        *bufInd++ = current + 4 - 4;
                        *bufInd++ = current + 6 - 4;
                        *bufInd++ = current + 1 - 4;
                        *bufInd++ = current + 6 - 4;
                        *bufInd++ = current + 3 - 4;
                    }
                    if (pi > 0)
                    { // add segment
                        *bufInd++ = current + 0;
                        *bufInd++ = current + 1;
                        *bufInd++ = current + 3;
                        *bufInd++ = current + 0;
                        *bufInd++ = current + 3;
                        *bufInd++ = current + 2;
                        current += 4;
                    }
                }
                current += 4; // make a gap
            }

            assert(bufPos == texBufHalf);
            assert(bufUps == (vec3f*)texBuffer.dataEnd());
            assert(bufInd == (uint16*)indBuffer.dataEnd());
        }

        // prepare the texture
        prepareTextureForLinesAndPoints(std::move(texBuffer), totalPoints);

        // prepare the mesh
        prepareMeshForLinesAndPoints(std::move(indBuffer), indicesCount);

        // prepare UBO
        {
            struct UboLineData
            {
                vec4f color;
                vec4f visibilities;
                vec4f typePlusUnitsPlusWidth;
            };
            UboLineData uboLineData;

            uboLineData.color = rawToVec4(spec.unionData.line.color);
            uboLineData.visibilities
                = rawToVec4(spec.commonData.visibilities);
            uboLineData.typePlusUnitsPlusWidth
                = vec4f((float)spec.type, (float)spec.unionData.line.units
                    , spec.unionData.line.width, 0.f);

            uniform = std::make_shared<UniformBuffer>();
            uniform->debugId = debugId;
            uniform->bind();
            uniform->load(uboLineData);
            info->gpuMemoryCost += sizeof(uboLineData);
        }
    }

    void loadPoint()
    {
        uint32 totalPoints = getTotalPoints();
        uint32 trianglesCount = totalPoints * 2;
        uint32 indicesCount = trianglesCount * 3;

        Buffer texBuffer;
        Buffer indBuffer;

        // prepare texture buffer and mesh indices
        {
            texBuffer.resize(totalPoints * sizeof(vec3f) * 2);
            vec3f *bufPos = (vec3f*)texBuffer.data();
            vec3f *bufUps = (vec3f*)texBuffer.data() + totalPoints;
            vec3f *texBufHalf = bufUps;
            (void)texBufHalf;

            indBuffer.resize(indicesCount * sizeof(uint16));
            uint16 *bufInd = (uint16*)indBuffer.data();
            uint16 current = 0;

            for (uint32 li = 0, lin = spec.positions.size();
                li < lin; li++)
            {
                const std::vector<std::array<float, 3>> &points
                    = spec.positions[li];
                for (uint32 pi = 0, pin = points.size();
                    pi < pin; pi++)
                {
                    vec3f p = rawToVec3(points[pi].data());
                    vec3f u = modelUp(p);
                    *bufPos++ = p;
                    *bufUps++ = u;
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 1;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 2;
                    current += 4;
                }
            }

            assert(bufPos == texBufHalf);
            assert(bufUps == (vec3f*)texBuffer.dataEnd());
            assert(bufInd == (uint16*)indBuffer.dataEnd());
        }

        // prepare the texture
        prepareTextureForLinesAndPoints(std::move(texBuffer), totalPoints);

        // prepare the mesh
        prepareMeshForLinesAndPoints(std::move(indBuffer), indicesCount);

        // prepare UBO
        {
            struct UboPointData
            {
                vec4f color;
                vec4f visibilities;
                vec4f typePlusRadius;
            };
            UboPointData uboPointData;

            uboPointData.color = rawToVec4(spec.unionData.point.color);
            uboPointData.visibilities
                = rawToVec4(spec.commonData.visibilities);
            uboPointData.typePlusRadius
                = vec4f((float)spec.type,
                (float)spec.unionData.point.radius
                    , 0.f, 0.f);

            uniform = std::make_shared<UniformBuffer>();
            uniform->debugId = debugId;
            uniform->bind();
            uniform->load(uboPointData);
            info->gpuMemoryCost += sizeof(uboPointData);
        }
    }
};

struct Word
{
    std::shared_ptr<Font> font;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    uint16 fileIndex;
    Word() : fileIndex(-1)
    {}
};

struct Text
{
    std::vector<Word> words;
    vec3 worldPosition;
    vec3f modelPosition;
    vec3f worldUp;

    Text() : worldPosition(0, 0, 0), modelPosition(0, 0, 0), worldUp(0, 0, 0)
    {}
};

struct TmpGlyph
{
    std::shared_ptr<Font> font;
    vec2f position;
    vec2f offset;
    float advance;
    uint16 glyphIndex;

    TmpGlyph() : position(0, 0), offset(0, 0), advance(0), glyphIndex(0)
    {}
};

struct TmpLine
{
    std::vector<TmpGlyph> glyphs;
    float width;
    float height;

    TmpLine() : width(0), height(0)
    {}
};

std::vector<TmpLine> textToGlyphs(
    const std::string &s,
    const std::vector<std::shared_ptr<Font>> &fontCascade)
{
    std::vector<TmpLine> lines;

    SBCodepointSequence codepoints
        = { SBStringEncodingUTF8, (void*)s.data(), s.length() };
    SBAlgorithmRef bidiAlgorithm
        = SBAlgorithmCreate(&codepoints);
    SBUInteger paragraphStart = 0;
    while (true)
    {
        SBParagraphRef paragraph
            = SBAlgorithmCreateParagraph(bidiAlgorithm,
                paragraphStart, INT32_MAX, SBLevelDefaultLTR);
        if (!paragraph)
            break;
        SBUInteger paragraphLength = SBParagraphGetLength(paragraph);
        SBUInteger lineStart = paragraphStart;
        paragraphStart += paragraphLength;

        while (true)
        {
            SBLineRef paragraphLine = SBParagraphCreateLine(paragraph,
                lineStart, paragraphLength);
            if (!paragraphLine)
                break;
            SBUInteger lineLength = SBLineGetLength(paragraphLine);
            lineStart += lineLength;
            paragraphLength -= lineLength;

            SBUInteger runCount = SBLineGetRunCount(paragraphLine);
            const SBRun *runArray = SBLineGetRunsPtr(paragraphLine);

            TmpLine line;
            line.glyphs.reserve(lineLength);

            for (uint32 runIndex = 0; runIndex < runCount; runIndex++)
            {
                const SBRun *run = runArray + runIndex;

                // bidi is running, harf is buzzing

                struct Buzz
                {
                    SBUInteger offset;
                    SBUInteger length;
                    uint32 font;
                    Buzz(const SBRun *run, sint32 o, sint32 l, uint32 f)
                        : offset(o), length(std::abs(l)), font(f)
                    {
                        (void)run;
                        assert(offset >= run->offset);
                        assert((sint64)offset + (sint64)length
                            <= (sint64)run->offset + (sint64)run->length);
                    }
                };

                std::list<Buzz> runs;
                runs.emplace_back(run, run->offset,
                    s[run->offset + run->length - 1] == '\n'
                    ? run->length - 1 : run->length, 0);

                while (!runs.empty())
                {
                    Buzz r = runs.front();
                    runs.pop_front();

                    bool terminal = r.font >= fontCascade.size();
                    std::shared_ptr<Font> fnt = fontCascade
                                    [terminal ? 0 : r.font];

                    hb_buffer_t *buffer = hb_buffer_create();
                    hb_buffer_add_utf8(buffer,
                        (char*)codepoints.stringBuffer,
                        codepoints.stringLength,
                        r.offset, r.length);
                    hb_buffer_set_direction(buffer, (run->level % 2) == 0
                        ? HB_DIRECTION_LTR : HB_DIRECTION_RTL);
                    hb_buffer_guess_segment_properties(buffer);
                    hb_shape(fnt->font, buffer, nullptr, 0);

                    uint32 len = hb_buffer_get_length(buffer);
                    hb_glyph_info_t *info
                        = hb_buffer_get_glyph_infos(buffer, nullptr);
                    hb_glyph_position_t *pos
                        = hb_buffer_get_glyph_positions(buffer, nullptr);

                    if (!terminal)
                    {
                        bool hasTofu = false;
                        for (uint32 i = 0; i < len; i++)
                        {
                            if (info[i].codepoint == 0)
                            {
                                hasTofu = true;
                                break;
                            }
                        }

                        if (hasTofu)
                        {
                            runs.reverse();
                            bool lastTofu = info[0].codepoint == 0;
                            sint32 lastCluster = info[0].cluster;
                            for (uint32 i = 1; i < len; i++)
                            {
                                bool currentTofu = info[i].codepoint == 0;
                                if (currentTofu != lastTofu)
                                {
                                    sint32 currentCluster
                                        = info[i].cluster;
                                    runs.emplace_back(run, lastCluster,
                                        currentCluster - lastCluster,
                                        r.font + lastTofu);
                                    lastCluster = currentCluster;
                                    lastTofu = currentTofu;
                                }
                            }
                            {
                                sint32 currentCluster
                                    = r.offset + r.length;
                                runs.emplace_back(run, lastCluster,
                                    currentCluster - lastCluster,
                                    r.font + lastTofu);
                            }
                            runs.reverse();
                            hb_buffer_destroy(buffer);
                            continue;
                        }
                    }

                    for (uint32 i = 0; i < len; i++)
                    {
                        assert(pos[i].y_advance == 0);
                        uint16 gi = info[i].codepoint;
                        assert(gi < fnt->glyphs.size());
                        TmpGlyph g;
                        g.font = fnt;
                        g.glyphIndex = gi;
                        g.advance = pos[i].x_advance / 64.f;
                        g.offset = vec2f(pos[i].x_offset,
                                pos[i].y_offset) / 64;
                        line.glyphs.push_back(g);
                    }

                    hb_buffer_destroy(buffer);
                }
            }

            lines.push_back(std::move(line));
            SBLineRelease(paragraphLine);
        }
        SBParagraphRelease(paragraph);
    }
    SBAlgorithmRelease(bidiAlgorithm);

    return lines;
}

vec2f textLayout(float maxWidth, float align, vec2f origin,
    std::vector<TmpLine> &lines)
{
    float w = 0;
    float h = 0;
    float maxAsc = 0;
    for (TmpLine &line : lines)
    {
        for (TmpGlyph &g : line.glyphs)
        {
            maxAsc = std::max(maxAsc, g.font->avgAsc);
            line.height = std::max(line.height, (float)g.font->size);
            const auto &f = g.font->glyphs[g.glyphIndex];
            float px = line.width + g.offset[0] + f.world[0];
            float py = h + g.offset[1] - f.world[3] - f.world[1];
            g.position = vec2f(px, py);
            line.width += g.advance;
        }
        line.height *= 1.5;
        h -= line.height;
        w = std::max(w, line.width);
    }

    // todo line wrap
    (void)maxWidth;

    // align
    for (TmpLine &line : lines)
    {
        float dx = (w - line.width) * align;
        for (TmpGlyph &g : line.glyphs)
            g.position[0] += dx;
    }

    // origin
    {
        vec2f dp = vec2f(w, h).cwiseProduct(origin) + vec2f(0, maxAsc);
        for (TmpLine &line : lines)
            for (TmpGlyph &g : line.glyphs)
                g.position -= dp;
    }

    return vec2f(w, h);
}

class GeodataText : public GeodataBase
{
public:
    std::vector<std::shared_ptr<Font>> fontCascade;
    std::vector<Text> texts;
    std::shared_ptr<UniformBuffer> uniform;

    std::vector<Word> generateWords(const std::string &s,
        float maxWidth, float align, vec2f origin)
    {
        std::vector<TmpLine> lines = textToGlyphs(s, fontCascade);
        textLayout(maxWidth, align, origin, lines);

        // sort into groups
        struct WordCompare
        {
            bool operator () (const Word &a, const Word &b) const
            {
                if (a.font == b.font)
                    return a.fileIndex < b.fileIndex;
                return a.font < b.font;
            }
        };
        std::map<Word, std::vector<TmpGlyph>, WordCompare> groups;
        for (TmpLine &line : lines)
        {
            for (const TmpGlyph &g : line.glyphs)
            {
                Word w;
                w.font = g.font;
                w.fileIndex = g.font->glyphs[g.glyphIndex].fileIndex;
                groups[w].push_back(g);
            }
        }

        // generate meshes
        std::vector<Word> words;
        for (auto &grp : groups)
        {
            GpuMeshSpec s;
            static const uint32 stride = sizeof(float) * 5;
            s.verticesCount = grp.second.size() * 4;
            s.indicesCount = grp.second.size() * 6;
            s.vertices.allocate(s.verticesCount * stride);
            s.indices.resize(s.indicesCount * sizeof(uint16));
            float *fit = (float*)s.vertices.data();
            uint16 *iit = (uint16*)s.indices.data();
            uint16 ii = 0;
            for (const auto &g : grp.second)
            {
                const auto &f = g.font->glyphs[g.glyphIndex];
                float px = g.position[0];
                float py = g.position[1];
                float pw = f.world[2];
                float ph = f.world[3];
                // 2--3
                // |  |
                // 0--1
                *fit++ = px; *fit++ = py; *fit++ = f.uvs[0]; *fit++ = f.uvs[1]; *(sint32*)fit++ = f.plane;
                *fit++ = px + pw; *fit++ = py; *fit++ = f.uvs[2]; *fit++ = f.uvs[1]; *(sint32*)fit++ = f.plane;
                *fit++ = px; *fit++ = py + ph; *fit++ = f.uvs[0]; *fit++ = f.uvs[3]; *(sint32*)fit++ = f.plane;
                *fit++ = px + pw; *fit++ = py + ph; *fit++ = f.uvs[2]; *fit++ = f.uvs[3]; *(sint32*)fit++ = f.plane;
                // 0-1-2
                // 1-3-2
                *iit++ = ii + 0; *iit++ = ii + 1; *iit++ = ii + 2;
                *iit++ = ii + 1; *iit++ = ii + 3; *iit++ = ii + 2;
                ii += 4;
            }
            assert(fit == (float*)s.vertices.dataEnd());
            assert(iit == (uint16*)s.indices.dataEnd());
            s.faceMode = GpuMeshSpec::FaceMode::Triangles;
            s.attributes[0].enable = true;
            s.attributes[0].components = 2;
            s.attributes[0].type = GpuTypeEnum::Float;
            s.attributes[0].offset = 0;
            s.attributes[0].stride = stride;
            s.attributes[1].enable = true;
            s.attributes[1].components = 2;
            s.attributes[1].type = GpuTypeEnum::Float;
            s.attributes[1].offset = sizeof(float) * 2;
            s.attributes[1].stride = stride;
            s.attributes[2].enable = true;
            s.attributes[2].components = 1;
            s.attributes[2].type = GpuTypeEnum::Int;
            s.attributes[2].offset = sizeof(float) * 4;
            s.attributes[2].stride = stride;
            Word w(grp.first);
            w.mesh = std::make_shared<Mesh>();
            ResourceInfo inf;
            w.mesh->load(inf, s, debugId);
            addMemory(inf);
            words.push_back(std::move(w));
        }
        return words;
    }

    void load(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp, const std::string &debugId) override
    {
        loadInit(renderer, info, specp, debugId);

        copyFonts();
        switch (spec.type)
        {
        case GpuGeodataSpec::Type::PointLabel:
            loadPointLabel();
            break;
        case GpuGeodataSpec::Type::LineLabel:
            loadLineLabel();
            break;
        default:
            throw std::invalid_argument("invalid geodata type");
        }

        loadFinish();
    }

    void copyFonts()
    {
        fontCascade.reserve(spec.fontCascade.size());
        for (auto &i : spec.fontCascade)
            fontCascade.push_back(std::static_pointer_cast<Font>(i));
    }

    void loadPointLabel()
    {
        assert(spec.texts.size() == spec.positions.size());
        for (uint32 i = 0, e = spec.texts.size(); i != e; i++)
        {
            float align = 0.5;
            switch (spec.unionData.pointLabel.textAlign)
            {
            case GpuGeodataSpec::TextAlign::Left:
                align = 0;
                break;
            case GpuGeodataSpec::TextAlign::Right:
                align = 1;
                break;
            default:
                break;
            }
            vec2f origin = vec2f(0.5, 1);
            switch (spec.unionData.pointLabel.origin)
            {
            case GpuGeodataSpec::Origin::TopLeft:
                origin = vec2f(0, 0);
                break;
            case GpuGeodataSpec::Origin::TopRight:
                origin = vec2f(1, 0);
                break;
            case GpuGeodataSpec::Origin::TopCenter:
                origin = vec2f(0.5, 0);
                break;
            case GpuGeodataSpec::Origin::CenterLeft:
                origin = vec2f(0, 0.5);
                break;
            case GpuGeodataSpec::Origin::CenterRight:
                origin = vec2f(1, 0.5);
                break;
            case GpuGeodataSpec::Origin::CenterCenter:
                origin = vec2f(0.5, 0.5);
                break;
            case GpuGeodataSpec::Origin::BottomLeft:
                origin = vec2f(0, 1);
                break;
            case GpuGeodataSpec::Origin::BottomRight:
                origin = vec2f(1, 1);
                break;
            case GpuGeodataSpec::Origin::BottomCenter:
                origin = vec2f(0.5, 1);
                break;
            default:
                break;
            }
            Text t;
            t.words = generateWords(spec.texts[i],
                spec.unionData.pointLabel.width,
                align, origin);
            t.modelPosition = rawToVec3(spec.positions[i][0].data());
            t.worldPosition = vec4to3(vec4(rawToMat4(spec.model)
                            * vec3to4(t.modelPosition, 1).cast<double>()));
            t.worldUp = worldUp(t.modelPosition);
            texts.push_back(std::move(t));
        }

        // prepare ubo
        {
            struct UboPointLabelData
            {
                vec4f colors[2];
                vec4f outline;
            };
            UboPointLabelData uboPointLabelData;

            uboPointLabelData.colors[0]
                = rawToVec4(spec.unionData.pointLabel.color);
            uboPointLabelData.colors[1]
                = rawToVec4(spec.unionData.pointLabel.color2);
            float os = std::sqrt(2) / spec.unionData.pointLabel.size;
            uboPointLabelData.outline
                = rawToVec4(spec.unionData.pointLabel.outline)
                .cwiseProduct(vec4f(1, 1, os, os));

            uniform = std::make_shared<UniformBuffer>();
            uniform->debugId = debugId;
            uniform->bind();
            uniform->load(uboPointLabelData);
            info->gpuMemoryCost += sizeof(uboPointLabelData);
        }
    }

    void loadLineLabel()
    {
        // todo
    }

    bool checkTextures()
    {
        bool ok = true;
        for (auto &t : texts)
        {
            for (auto &w : t.words)
            {
                if (w.texture)
                    continue;
                w.texture = std::static_pointer_cast<Texture>(
                    w.font->fontHandle->requestTexture(w.fileIndex));
                ok = ok && !!w.texture;
            }
        }
        return ok;
    }
};

} // namespace

namespace priv
{

bool RendererImpl::geodataTestVisibility(
    const float visibility[4],
    const vec3 &pos, const vec3f &up)
{
    vec3 eye = rawToVec3(draws->camera.eye);
    double distance = length(vec3(eye - pos));
    if (visibility[0] == visibility[0] && distance > visibility[0])
        return false;
    distance *= 2 / draws->camera.proj[5];
    if (visibility[1] == visibility[1] && distance < visibility[1])
        return false;
    if (visibility[2] == visibility[2] && distance > visibility[2])
        return false;
    vec3f f = normalize(vec3(eye - pos)).cast<float>();
    if (visibility[3] == visibility[3]
        && dot(f, up) < visibility[3])
        return false;
    return true;
}

void RendererImpl::initializeGeodata()
{
    // load shader geodata line
    {
        shaderGeodataLine = std::make_shared<Shader>();
        shaderGeodataLine->debugId
            = "data/shaders/geodataLine.*.glsl";
        shaderGeodataLine->loadInternal(
            "data/shaders/geodataLine.vert.glsl",
            "data/shaders/geodataLine.frag.glsl");
        shaderGeodataLine->bindTextureLocations({
                { "texLineData", 0 }
            });
        shaderGeodataLine->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboLineData", 2 }
            });
    }

    // load shader geodata point
    {
        shaderGeodataPoint = std::make_shared<Shader>();
        shaderGeodataPoint->debugId
            = "data/shaders/geodataPoint.*.glsl";
        shaderGeodataPoint->loadInternal(
            "data/shaders/geodataPoint.vert.glsl",
            "data/shaders/geodataPoint.frag.glsl");
        shaderGeodataPoint->bindTextureLocations({
                { "texPointData", 0 }
            });
        shaderGeodataPoint->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboPointData", 2 }
            });
    }

    // load shader geodata point label
    {
        shaderGeodataPointLabel = std::make_shared<Shader>();
        shaderGeodataPointLabel->debugId
            = "data/shaders/geodataPointLabel.*.glsl";
        shaderGeodataPointLabel->loadInternal(
            "data/shaders/geodataPointLabel.vert.glsl",
            "data/shaders/geodataPointLabel.frag.glsl");
        shaderGeodataPointLabel->loadUniformLocations({
                "uniPosition",
                "uniScale",
                "uniPass"
            });
        shaderGeodataPointLabel->bindTextureLocations({
                { "texGlyphs", 0 }
            });
        shaderGeodataPointLabel->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboPointLabelData", 2 }
            });
    }

    uboGeodataCamera = std::make_shared<UniformBuffer>();
    uboGeodataCamera->debugId = "uboGeodataCamera";
    uboGeodataView = std::make_shared<UniformBuffer>();
    uboGeodataView->debugId = "uboGeodataView";

    CHECK_GL("initialize geodata");
}

void RendererImpl::initializeZBufferOffsetValues(
        vec3 &zBufferOffsetValues,
        mat4 &davidProj, mat4 &davidProjInv)
{
    vec3 up1 = normalize(rawToVec3(draws->camera.eye)); // todo projected systems
    vec3 up2 = vec4to3(vec4(viewInv * vec4(0, 1, 0, 0)));
    double tiltFactor = std::acos(std::max(
        dot(up1, up2), 0.0)) / M_PI_2;
    double distance = draws->camera.tagretDistance;
    double distanceFactor = 1 / std::max(1.0,
        std::log(distance) / std::log(1.04));
    zBufferOffsetValues = vec3(1, distanceFactor, tiltFactor);

    // here comes the anti-david trick
    // note: David is the developer behind vts-browser-js
    //          he also designed the geodata styling rules
    // unfortunately, he designed zbuffer-offset in a way
    //   that only works with his perspective projection
    // therefore we simulate his projection,
    //   apply the offset and undo his projection
    // this way the parameters work as expected
    //   and the values already in z-buffer are valid too

    double factor = std::max(draws->camera.altitudeOverEllipsoid,
        draws->camera.tagretDistance) / 600000;
    double davidNear = std::max(2.0, factor * 40);
    double davidFar = 600000 * std::max(1.0, factor) * 20;

    // this decomposition works with the most basic projection matrix only
    double fov = 2 * radToDeg(std::atan(1 / proj(1, 1)));
    double aspect = proj(1, 1) / proj(0, 0);
    double myNear = proj(2, 3) / (proj(2, 2) - 1);
    double myFar = proj(2, 3) / (proj(2, 2) + 1);
    (void)myNear;
    (void)myFar;

    assert(perspectiveMatrix(fov, aspect,
        myNear, myFar).isApprox(proj, 1e-10));

    davidProj = perspectiveMatrix(fov, aspect,
        davidNear, davidFar);
    davidProjInv = davidProj.inverse();
}

void RendererImpl::renderGeodata()
{
    //glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // z-buffer-offset global data
    vec3 zBufferOffsetValues;
    mat4 davidProj, davidProjInv;
    initializeZBufferOffsetValues(zBufferOffsetValues,
        davidProj, davidProjInv);

    // initialize camera data
    {
        struct UboCameraData
        {
            mat4f proj;
            vec4f cameraParams; // screen width in pixels, screen height in pixels, view extent in meters
        } ubo;

        ubo.proj = proj.cast<float>();
        ubo.cameraParams = vec4f(widthPrev, heightPrev,
                draws->camera.viewExtent, 0);

        uboGeodataCamera->bind();
        uboGeodataCamera->load(ubo);
        uboGeodataCamera->bindToIndex(0);
    }

    std::sort(draws->geodata.begin(), draws->geodata.end(),
        [](const DrawGeodataTask &a, const DrawGeodataTask &b)
    {
        return ((GeodataBase*)a.geodata.get())->spec.commonData.zIndex
            < ((GeodataBase*)b.geodata.get())->spec.commonData.zIndex;
    });

    // iterate over geodata
    for (const DrawGeodataTask &t : draws->geodata)
    {
        GeodataBase *gg = (GeodataBase*)t.geodata.get();

        mat4 depthOffsetProj;
        {
            vec3 zbo = rawToVec3(gg->spec.commonData.zBufferOffset)
                .cast<double>();
            double off = dot(zbo, zBufferOffsetValues) * 0.0001;
            double off2 = (off + 1) * 2 - 1;
            mat4 s = scaleMatrix(vec3(1, 1, off2));
            // apply anti-david measures
            depthOffsetProj = proj * davidProjInv * s * davidProj;
        }

        mat4 model = rawToMat4(gg->spec.model);
        mat4 mv = view * model;
        mat4 mvp = depthOffsetProj * mv;
        mat4 mvInv = mv.inverse();
        mat4 mvpInv = mvp.inverse();

        // view ubo
        {
            struct UboViewData
            {
                mat4f mvp;
                mat4f mvpInv;
                mat4f mv;
                mat4f mvInv;
            } ubo;

            ubo.mvp = mvp.cast<float>();
            ubo.mvpInv = mvpInv.cast<float>();
            ubo.mv = mv.cast<float>();
            ubo.mvInv = mvInv.cast<float>();

            uboGeodataView->bind();
            uboGeodataView->load(ubo);
            uboGeodataView->bindToIndex(1);
        }

        switch (gg->spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineFlat:
        {
            GeodataGeometry *g = static_cast<GeodataGeometry*>(gg);
            shaderGeodataLine->bind();
            g->uniform->bindToIndex(2);
            g->texture->bind();
            Mesh *msh = g->mesh.get();
            msh->bind();
            glEnable(GL_STENCIL_TEST);
            msh->dispatch();
            glDisable(GL_STENCIL_TEST);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            //msh->dispatch();
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } break;
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointFlat:
        {
            GeodataGeometry *g = static_cast<GeodataGeometry*>(gg);
            shaderGeodataPoint->bind();
            g->uniform->bindToIndex(2);
            g->texture->bind();
            Mesh *msh = g->mesh.get();
            msh->bind();
            glEnable(GL_STENCIL_TEST);
            msh->dispatch();
            glDisable(GL_STENCIL_TEST);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            //msh->dispatch();
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } break;
        case GpuGeodataSpec::Type::PointLabel:
        {
            glEnable(GL_BLEND);
            GeodataText *g = static_cast<GeodataText*>(gg);
            if (!g->checkTextures())
                continue;
            shaderGeodataPointLabel->bind();
            g->uniform->bindToIndex(2);
            std::shared_ptr<Texture> lastTexture;
            for (auto &t : g->texts)
            {
                if (!geodataTestVisibility(
                    g->spec.commonData.visibilities,
                    t.worldPosition, t.worldUp))
                    continue;
                shaderGeodataPointLabel->uniformVec3(0,
                    t.modelPosition.data());
                for (int pass = 0; pass < 2; pass++)
                {
                    shaderGeodataPointLabel->uniform(2, pass);
                    for (auto &w : t.words)
                    {
                        shaderGeodataPointLabel->uniform(1, options.textScale
                           * g->spec.unionData.pointLabel.size / w.font->size);
                        if (w.texture != lastTexture)
                        {
                            w.texture->bind();
                            lastTexture = w.texture;
                        }
                        w.mesh->bind();
                        w.mesh->dispatch();
                    }
                }
            }
            glDisable(GL_BLEND);
        } break;
        default:
        {
        } break;
        }
    }

    glDepthMask(GL_TRUE);
    //glEnable(GL_CULL_FACE);
}

} // namespace priv

void Renderer::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec,
    const std::string &debugId)
{
    switch (spec.type)
    {
    case GpuGeodataSpec::Type::PointLabel:
    case GpuGeodataSpec::Type::LineLabel:
    {
        auto r = std::make_shared<GeodataText>();
        r->load(&*impl, info, spec, debugId);
        info.userData = r;
    } break;
    default:
    {

        auto r = std::make_shared<GeodataGeometry>();
        r->load(&*impl, info, spec, debugId);
        info.userData = r;
    } break;
    }
}

} } // namespace vts renderer

