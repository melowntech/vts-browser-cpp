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

struct BidiAlgorithm
{
    SBAlgorithmRef algorithm;

    BidiAlgorithm(SBCodepointSequence *codepoints) : algorithm(nullptr)
    {
        algorithm = SBAlgorithmCreate(codepoints);
    }

    ~BidiAlgorithm()
    {
        SBAlgorithmRelease(algorithm);
    }

    SBAlgorithmRef operator () () { return algorithm; };
};

struct BidiParagraph
{
    SBParagraphRef paragraph;

    BidiParagraph(SBAlgorithmRef bidiAlgorithm, SBUInteger paragraphStart)
        : paragraph(nullptr)
    {
        paragraph = SBAlgorithmCreateParagraph(bidiAlgorithm,
                paragraphStart, INT32_MAX, SBLevelDefaultLTR);
    }

    ~BidiParagraph()
    {
        SBParagraphRelease(paragraph);
    }

    SBParagraphRef operator () () { return paragraph; };
};

struct BidiLine
{
    SBLineRef paragraphLine;

    BidiLine(SBParagraphRef paragraph, SBUInteger lineStart,
        SBUInteger paragraphLength) : paragraphLine(nullptr)
    {
        paragraphLine = SBParagraphCreateLine(paragraph,
            lineStart, paragraphLength);
    }

    ~BidiLine()
    {
        SBLineRelease(paragraphLine);
    }

    SBLineRef operator () () { return paragraphLine; };
};

struct HarfBuffer
{
    hb_buffer_t *buffer;

    HarfBuffer() : buffer(nullptr)
    {
        buffer = hb_buffer_create();
    }

    ~HarfBuffer()
    {
        hb_buffer_destroy(buffer);
    }

    hb_buffer_t *operator () () { return buffer; };
};

// append B to the end of A
void appendLine(TmpLine &a, TmpLine &&b)
{
    for (auto &it : b.glyphs)
        a.glyphs.push_back(std::move(it));
}

TmpLine textRunToGlyphs(
    const std::string &s,
    const std::vector<std::shared_ptr<Font>> &fontCascade,
    SBUInteger offset, SBUInteger length, uint32 fontIndex,
    bool rtl)
{
    while (length && s[offset + length - 1] == '\n')
        length--;

    if (length == 0)
        return {};

    bool terminal = fontIndex >= fontCascade.size();
    std::shared_ptr<Font> fnt = fontCascade[terminal ? 0 : fontIndex];

    HarfBuffer buffer;
    hb_buffer_add_utf8(buffer(),
        s.data(), s.length(), offset, length);
    hb_buffer_set_direction(buffer(),
        rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    hb_buffer_guess_segment_properties(buffer());
    hb_shape(fnt->font, buffer(), nullptr, 0);

    uint32 len = hb_buffer_get_length(buffer());
    hb_glyph_info_t *info
        = hb_buffer_get_glyph_infos(buffer(), nullptr);
    hb_glyph_position_t *pos
        = hb_buffer_get_glyph_positions(buffer(), nullptr);

    TmpLine line;
    line.glyphs.reserve(len);

    if (!terminal)
    {
        // we have another font available to use
        // so we search for tofus
        bool hasTofu = false;
        for (uint32 i = 0; i < len; i++)
        {
            if (info[i].codepoint == 0)
            {
                hasTofu = true;
                break;
            }
        }

        // if any tofu was found, split the text into parts
        // each part is either all tofu or no tofu
        if (hasTofu)
        {
            // RTL text has clusters in reverse order
            //   reverse it back to simplify the algorithm
            if (rtl)
                std::reverse(info, info + len);

            bool lastTofu = info[0].codepoint == 0;
            bool hadTofu = lastTofu;
            uint32 lastCluster = info[0].cluster;
            for (uint32 i = 1; i < len; i++)
            {
                bool currentTofu = info[i].codepoint == 0;
                if (currentTofu != lastTofu)
                {
                    // there are rare situations where harfbuzz
                    //   puts some tofu and non-tofu
                    //   into same cluster,
                    //   which prevents me to split them
                    // for a lack of better ideas
                    //   we send these sequences to next font
                    //   without splitting
                    uint32 currentCluster = info[i].cluster;
                    if (currentCluster != lastCluster)
                    {
                        appendLine(line, textRunToGlyphs(s, fontCascade,
                            lastCluster, currentCluster - lastCluster,
                            fontIndex + hadTofu, rtl));
                        lastCluster = currentCluster;
                        hadTofu = currentTofu;
                    }
                    lastTofu = currentTofu;
                    hadTofu = hadTofu || currentTofu;
                }
            }
            {
                uint32 currentCluster = offset + length;
                appendLine(line, textRunToGlyphs(s, fontCascade,
                    lastCluster, currentCluster - lastCluster,
                    fontIndex + hadTofu, rtl));
            }
            return line;
        }
    }

    // add glyphs information to output
    for (uint32 i = 0; i < len; i++)
    {
        assert(pos[i].y_advance == 0);
        uint16 gi = info[i].codepoint;
        assert(gi < fnt->glyphs.size());
        TmpGlyph g;
        g.font = fnt;
        g.glyphIndex = gi;
        g.advance = pos[i].x_advance / 64.f;
        g.offset = vec2f(pos[i].x_offset, pos[i].y_offset) / 64;
        line.glyphs.push_back(g);
    }
    return line;
}

std::vector<TmpLine> textToGlyphs(
    const std::string &s,
    const std::vector<std::shared_ptr<Font>> &fontCascade)
{
    std::vector<TmpLine> lines;
    SBCodepointSequence codepoints
        = { SBStringEncodingUTF8, (void*)s.data(), s.length() };
    BidiAlgorithm bidiAlgorithm(&codepoints);
    SBUInteger paragraphStart = 0;
    while (true)
    {
        BidiParagraph paragraph(bidiAlgorithm(), paragraphStart);
        if (!paragraph())
            break;

        SBUInteger paragraphLength = SBParagraphGetLength(paragraph());
        SBUInteger lineStart = paragraphStart;
        paragraphStart += paragraphLength;

        while (true)
        {
            BidiLine paragraphLine(paragraph(), lineStart, paragraphLength);
            if (!paragraphLine())
                break;

            SBUInteger lineLength = SBLineGetLength(paragraphLine());
            lineStart += lineLength;
            paragraphLength -= lineLength;

            SBUInteger runCount = SBLineGetRunCount(paragraphLine());
            const SBRun *runArray = SBLineGetRunsPtr(paragraphLine());

            TmpLine line;
            line.glyphs.reserve(lineLength * 2 + 5);
            for (uint32 runIndex = 0; runIndex < runCount; runIndex++)
            {
                const SBRun *run = runArray + runIndex;
                appendLine(line, textRunToGlyphs(s, fontCascade,
                    run->offset, run->length, 0, (run->level % 2) == 1));
            }
            if (!line.glyphs.empty())
                lines.push_back(std::move(line));
        }
    }
    return lines;
}

vec2f textLayout(float size, float align,
    std::vector<TmpLine> &lines)
{
    // find glyph positions
    vec2f res = vec2f(0, 0);
    {
        vec2f o = vec2f(0, 0);
        for (TmpLine &line : lines)
        {
            for (TmpGlyph &g : line.glyphs)
            {
                const auto &f = g.font->glyphs[g.glyphIndex];
                float s = size / g.font->size;
                g.position = o + (g.offset + f.offset) * s;
                g.size = f.size * s;
                o[0] += g.advance * s;
            }
            line.width = o[0];
            res[0] = std::max(res[0], o[0]);
            o[0] = 0;
            o[1] += size * -1.5; // 1.5 line spacing
        }
        res[1] = -o[1];
    }

    // center the entire text
    vec2f off = res.cwiseProduct(vec2f(-0.5, 0.5)) + vec2f(0, -size);
    for (TmpLine &line : lines)
        for (TmpGlyph &g : line.glyphs)
            g.position += off;

    // align (move each line independently)
    for (TmpLine &line : lines)
    {
        float dx = (res[0] - line.width) * align;
        for (TmpGlyph &g : line.glyphs)
            g.position[0] += dx;
    }

    return res;
}

Rect textCollision(const std::vector<TmpLine> &lines)
{
    Rect collision;
    for (const TmpLine &line : lines)
    {
        for (const TmpGlyph &g : line.glyphs)
            collision = merge(collision,
                Rect(g.position, g.position + g.size));
    }
    return collision;
}

Text generateTexts(std::vector<TmpLine> &lines)
{
    Text text;
    text.coordinates.reserve(500);
    uint32 vertexIndex = 0;
    Subtext st;
    for (const TmpLine &line : lines)
    {
        for (const TmpGlyph &tg : line.glyphs)
        {
            auto fileIndex = tg.font->glyphs[tg.glyphIndex].fileIndex;
            if (st.font != tg.font || st.fileIndex != fileIndex)
            {
                if (st.indicesCount > 0)
                    text.subtexts.push_back(std::move(st));
                st = Subtext();
                st.font = tg.font;
                st.fileIndex = fileIndex;
                st.indicesStart = vertexIndex;
            }

            // 2--3
            // |  |
            // 0--1
            const Glyph &g = tg.font->glyphs[tg.glyphIndex];
            for (int i = 0; i < 4; i++)
            {
                vec4f c;
                c[0] = tg.position[0] + ((i % 2) == 1 ? tg.size[0] : 0);
                c[1] = tg.position[1] + ((i / 2) == 1 ? tg.size[1] : 0);
                c[2] = g.uvs[0 + 2 * (i % 2)];
                c[3] = g.uvs[1 + 2 * (i / 2)];
                c[2] += g.plane * 2;
                text.coordinates.push_back(c);
            }
            st.indicesCount += 6;
            vertexIndex += 6;

            // todo better handle shader limit
            if (text.coordinates.size() >= 480)
            {
                text.subtexts.push_back(std::move(st));
                return text;
            }
        }
    }
    if (st.indicesCount > 0)
        text.subtexts.push_back(std::move(st));
    text.coordinates.shrink_to_fit();
    return text;
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

template<class T>
T interpFactor(T what, T before, T after)
{
    return (what - before) / (after - before);
}

template<class T>
void arrayPosition(const std::vector<T> &v, T p, uint32 &index, T &factor)
{
    assert(v.size() > 1);
    if (p <= v[0])
    {
        index = 0;
        factor = 0;
        return;
    }
    if (p >= v[v.size() - 1])
    {
        index = v.size() - 2;
        factor = 1;
        return;
    }
    assert(index < v.size() - 1);
    while (v[index] >= p)
        index--;
    while (v[index + 1] <= p)
        index++;
    factor = interpFactor(p, v[index], v[index + 1]);
}

void textLinePositions(GeodataTile *g,
    const std::vector<std::array<float, 3>> &positions, Text &t)
{
    mat4 model = rawToMat4(g->spec.model);
    const auto &m2w = [&](const std::array<float, 3> &pos) -> vec3
    {
        return vec4to3(vec4(model
            * vec3to4(rawToVec3(pos.data()), 1).cast<double>()));
    };

    // precompute absolute distances
    std::vector<double> dists;
    dists.reserve(positions.size());
    double totalDist = 0;
    {
        vec3 lastPos = m2w(positions[0]);
        for (const auto &it : positions)
        {
            vec3 p = m2w(it);
            totalDist += length(vec3(lastPos - p));
            dists.push_back(totalDist);
            lastPos = p;
        }
        assert(totalDist > 1e-7);
    }

    // normalize distance-along-the-line into -1..1 range
    std::vector<float> &lvp = t.lineVertPositions;
    lvp.reserve(positions.size());
    for (uint32 i = 0, e = positions.size(); i != e; i++)
        lvp.push_back(2 * dists[i] / totalDist - 1);
    assert(std::abs(lvp[0] + 1) < 1e-7);
    assert(std::abs(lvp[lvp.size() - 1] - 1) < 1e-7);
    assert(std::is_sorted(lvp.begin(), lvp.end()));

    // find line-positions for each glyph
    uint32 gc = t.coordinates.size() / 4;
    t.lineGlyphPositions.reserve(gc);
    for (uint32 i = 0; i < gc; i++)
    {
        float x = 0;
        for (uint32 j = 0; j < 4; j++)
            x += t.coordinates[i * 4 + j][0];
        x *= 0.25;
        for (uint32 j = 0; j < 4; j++)
            t.coordinates[i * 4 + j][0] -= x;
        float lgp = 2 * (x + totalDist * 0.5) / totalDist - 1;
        t.lineGlyphPositions.push_back(lgp);
    }
    //assert(std::is_sorted(t.lineGlyphPositions.begin(),
    //    t.lineGlyphPositions.end()));

    // find center of the line
    {
        uint32 i = 0;
        float f;
        arrayPosition<float>(lvp, 0, i, f);
        Point t;
        t.modelPosition = interpolate(rawToVec3(positions[i].data()),
            rawToVec3(positions[i + 1].data()), (float)f);
        t.worldPosition = interpolate(m2w(positions[i]),
            m2w(positions[i + 1]), (double)f);
        t.worldUp = normalize(t.worldPosition).cast<float>();
        g->points.push_back(t);
    }
}

float compensatePerspective(const RenderViewImpl *rv,
    const GeodataJob &j)
{
    double mydist = length(vec3(j.worldPosition()
        - rawToVec3(rv->draws->camera.eye)));
    double refdist = rv->draws->camera.tagretDistance;
    return mydist / refdist;
}

float labelFlatScale(const RenderViewImpl *rv,
    const GeodataJob &j, bool &allFit)
{
    const auto &g = j.g;
    auto &t = g->texts[j.itemIndex];
    float scale = rv->options.textScale;
    if (g->spec.unionData.labelFlat.units
        == GpuGeodataSpec::Units::Pixels)
    {
        scale *= rv->draws->camera.viewExtent / rv->height;
        scale *= compensatePerspective(rv, j);
    }
    else
    {
        scale *= g->spec.unionData.labelFlat.size / 25;
    }

    {
        // smaller scale if the original scale would not fit
        const float m = 1 / std::max(-t.lineGlyphPositions[0],
            t.lineGlyphPositions[t.lineGlyphPositions.size() - 1]);
        allFit = scale < m;
        if (!allFit)
            scale = m;
    }

    {
        // negative scale if the text is reversed
        const auto &mps = g->spec.positions[j.itemIndex];
        mat4 mvp = rv->viewProj * g->model;
        vec3 sa = vec4to3(vec4(mvp * vec3to4(rawToVec3(
            mps.rbegin()->data()).cast<double>(), 1.0)), true);
        vec3 sb = vec4to3(vec4(mvp * vec3to4(rawToVec3(
            mps.begin()->data()).cast<double>(), 1.0)), true);
        if (sa[0] < sb[0])
            scale *= -1;
    }
    return scale;
}

} // namespace

void GeodataTile::copyFonts()
{
    fontCascade.reserve(spec.fontCascade.size());
    for (auto &i : spec.fontCascade)
        fontCascade.push_back(std::static_pointer_cast<Font>(i));
}

void GeodataTile::loadLabelScreens()
{
    assert(spec.texts.size() == spec.positions.size());
    copyPoints();
    copyFonts();

    float align = numericAlign(spec.unionData.labelScreen.textAlign);
    for (uint32 i = 0, e = spec.texts.size(); i != e; i++)
    {
        std::vector<TmpLine> lines = textToGlyphs(
            spec.texts[i], fontCascade);
        vec2f originSize = textLayout(
            spec.unionData.labelScreen.size,
            align, lines);
        Text t = generateTexts(lines);
        t.size = spec.unionData.labelScreen.size;
        t.collision = textCollision(lines);
        t.originSize = originSize;
        info->ramMemoryCost += t.coordinates.size() * sizeof(vec4f);
        info->ramMemoryCost += t.subtexts.size() * sizeof(Subtext);
        texts.push_back(std::move(t));
    }
    info->ramMemoryCost += texts.size() * sizeof(decltype(texts[0]));
}

void GeodataTile::loadLabelFlats()
{
    assert(spec.texts.size() == spec.positions.size());
    copyFonts();
    points.reserve(spec.positions.size());
    for (uint32 i = 0, e = spec.texts.size(); i != e; i++)
    {
        assert(spec.positions[i].size() > 1); // line must have at least two points
        std::vector<TmpLine> lines = textToGlyphs(
            spec.texts[i], fontCascade);
        float size = spec.unionData.labelFlat.units
            == GpuGeodataSpec::Units::Meters
            ? 25 : spec.unionData.labelFlat.size;
        textLayout(size, 0.5, lines); // align to center
        assert(lines.size() == 1); // flat labels may not be multi-line
        Text t = generateTexts(lines);
        t.size = size;
        textLinePositions(this, spec.positions[i], t);
        info->ramMemoryCost += t.coordinates.size() * sizeof(vec4f);
        info->ramMemoryCost += t.subtexts.size() * sizeof(Subtext);
        texts.push_back(std::move(t));
    }
    info->ramMemoryCost += texts.size() * sizeof(decltype(texts[0]));
    assert(points.size() == spec.positions.size());
}

bool GeodataTile::checkTextures()
{
    bool ok = true;
    for (auto &t : texts)
    {
        for (auto &w : t.subtexts)
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

bool regenerateJobLabelFlat(const RenderViewImpl *rv, GeodataJob &j)
{
    const auto &g = j.g;
    auto &t = g->texts[j.itemIndex];
    const auto &mps = g->spec.positions[j.itemIndex];
    const mat4f mvp = (rv->viewProj * g->model).cast<float>();
    t.collisionGlyphsRects.clear();
    t.collisionGlyphsRects.reserve(t.lineGlyphPositions.size());
    bool allFit;
    float scale = labelFlatScale(rv, j, allFit);
    uint32 vi = 0;
    float cs1 = t.size * std::abs(scale) / compensatePerspective(rv, j) / rv->draws->camera.viewExtent * rv->height;
    vec2f cs = vec2f(cs1 / rv->width, cs1 / rv->height);
    for (uint32 i = 0, e = t.lineGlyphPositions.size(); i != e; i++)
    {
        float lp = t.lineGlyphPositions[i] * scale;
        float f;
        arrayPosition(t.lineVertPositions, lp, vi, f);
        vec3f mpa = rawToVec3(mps[vi].data());
        vec3f mpb = rawToVec3(mps[vi + 1].data());
        vec3f mp = interpolate(mpa, mpb, f);
        vec4f cp4 = mvp * vec3to4(mp, 1);
        vec2f cp = vec4to2(cp4, true);
        t.collisionGlyphsRects.push_back(Rect(cp - cs, cp + cs));
    }
    return allFit;
}

void preDrawJobLabelFlat(const RenderViewImpl *rv, const GeodataJob &j,
    std::vector<vec3> &worldPos, float &scale)
{
    const auto &g = j.g;
    auto &t = g->texts[j.itemIndex];
    const auto &mps = g->spec.positions[j.itemIndex];
    worldPos.clear();
    worldPos.reserve(t.lineGlyphPositions.size());
    bool dummyAllFit;
    scale = labelFlatScale(rv, j, dummyAllFit);
    uint32 vi = 0;
    for (uint32 i = 0, e = t.lineGlyphPositions.size(); i != e; i++)
    {
        float lp = t.lineGlyphPositions[i] * scale;
        float f;
        arrayPosition(t.lineVertPositions, lp, vi, f);
        vec3f mpa = rawToVec3(mps[vi].data());
        vec3f mpb = rawToVec3(mps[vi + 1].data());
        vec3f mp = interpolate(mpa, mpb, f);
        vec3 wp = vec4to3(vec4(g->model * (vec3to4(mp, 1).cast<double>())));
        worldPos.push_back(wp);
    }
    scale = std::abs(scale);
}

} } // namespace vts renderer

