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

#include <list>
#include <map>

#include "geodata.hpp"
#include "font.hpp"

extern "C"
{
#include <SheenBidi.h>
}

namespace vts { namespace renderer
{

namespace
{

struct TmpGlyph
{
    std::shared_ptr<Font> font;
    vec2f position; // screen units
    vec2f size; // screen units
    vec2f offset; // font units
    float advance; // font units
    uint16 glyphIndex;

    TmpGlyph() : position(0, 0), offset(0, 0), advance(0), glyphIndex(0)
    {}
};

struct TmpLine
{
    std::vector<TmpGlyph> glyphs;
    float width; // screen units

    TmpLine() : width(0)
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

void mergeRect(vec2f &ro, vec2f &rs, const vec2f &go, const vec2f &gs)
{
    assert(gs[0] >= 0 && gs[1] >= 0);
    if (!(ro[0] == ro[0]))
    {
        ro = go;
        rs = gs;
        return;
    }
    assert(rs[0] >= 0 && rs[1] >= 0);
    for (int i = 0; i < 2; i++)
    {
        float a = std::min(ro[i], go[i]);
        float b = std::max(ro[i] + rs[i], go[i] + gs[i]);
        ro[i] = a;
        rs[i] = b - a;
    }
}

vec2f textLayout(float size, float maxWidth, float align, vec2f origin,
    std::vector<TmpLine> &lines)
{
    // find glyph positions
    {
        vec2f o = vec2f(0, 0);
        for (TmpLine &line : lines)
        {
            float ml = 0;
            for (TmpGlyph &g : line.glyphs)
            {
                ml = std::max(ml, (float)g.font->size);
                const auto &f = g.font->glyphs[g.glyphIndex];
                g.position = (o + g.offset + f.offset) * size / g.font->size;
                g.size = f.size * size / g.font->size;
                o[0] += g.advance;
            }
            o[0] = 0;
            o[1] -= ml * 1.5;
        }
    }

    // todo line wrap
    (void)maxWidth;

    // find text bounding box
    vec2f ro = nan2().cast<float>(), rs = nan2().cast<float>();
    for (TmpLine &line : lines)
    {
        vec2f lo = nan2().cast<float>(), ls = nan2().cast<float>();
        for (TmpGlyph &g : line.glyphs)
            mergeRect(lo, ls, g.position, g.size);
        line.width = ls[0] - lo[0];
        mergeRect(ro, rs, lo, ls); // merge line box into global box
    }

    // subtract bounding box origin
    for (TmpLine &line : lines)
        for (TmpGlyph &g : line.glyphs)
            g.position -= ro;
    ro = vec2f(0, 0);

    // origin (move all text as whole)
    {
        vec2f dp = rs.cwiseProduct(vec2f(-1, 1)).cwiseProduct(origin)
            + rs.cwiseProduct(vec2f(0, -1));
        for (TmpLine &line : lines)
            for (TmpGlyph &g : line.glyphs)
                g.position += dp;
    }

    // align (move each line independently)
    for (TmpLine &line : lines)
    {
        float dx = (rs[0] - line.width) * align;
        for (TmpGlyph &g : line.glyphs)
            g.position[0] += dx;
    }

    return rs;
}

void findRect(Text &t, vec2f origin, const vec2f &size, const vec2f &margin)
{
    origin[1] = 1 - origin[1];
    t.rectOrigin = -origin.cwiseProduct(size);
    t.rectSize = size;
    t.rectOrigin -= margin;
    t.rectSize += 2 * margin;
}

std::vector<Word> generateMeshes(std::vector<TmpLine> &lines,
    float align, const vec2f &origin,
    const std::string &debugId)
{
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
            float pw = g.size[0];
            float ph = g.size[1];
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
        words.push_back(std::move(w));
    }
    return words;
}

float numericAlign(GpuGeodataSpec::TextAlign a)
{
    float align = 0.5;
    switch (a)
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
    return align;
}

vec2f numericOrigin(GpuGeodataSpec::Origin o)
{
    vec2f origin = vec2f(0.5, 1);
    switch (o)
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
    return origin;
}

} // namespace

void GeodataText::load(RendererImpl *renderer, ResourceInfo &info,
    GpuGeodataSpec &specp, const std::string &debugId)
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

void GeodataText::copyFonts()
{
    fontCascade.reserve(spec.fontCascade.size());
    for (auto &i : spec.fontCascade)
        fontCascade.push_back(std::static_pointer_cast<Font>(i));
}

void GeodataText::loadPointLabel()
{
    assert(spec.texts.size() == spec.positions.size());
    float align = numericAlign(spec.unionData.pointLabel.textAlign);
    vec2f origin = numericOrigin(spec.unionData.pointLabel.origin);
    for (uint32 i = 0, e = spec.texts.size(); i != e; i++)
    {
        std::vector<TmpLine> lines = textToGlyphs(
            spec.texts[i], fontCascade);
        vec2f rectSize = textLayout(
            spec.unionData.pointLabel.size,
            spec.unionData.pointLabel.width,
            align, origin, lines);
        Text t;
        t.words = generateMeshes(lines, align, origin, debugId);
        findRect(t, origin, rectSize,
            rawToVec2(spec.unionData.pointLabel.margin));
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

void GeodataText::loadLineLabel()
{
    // todo
}

bool GeodataText::checkTextures()
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

} } // namespace vts renderer

