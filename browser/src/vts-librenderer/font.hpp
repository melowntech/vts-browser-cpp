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

#ifndef FONT_HPP_sdr7f7jk4
#define FONT_HPP_sdr7f7jk4

#include <hb-ft.h>

namespace vts { namespace renderer
{

struct Glyph
{
    vec4f uvs; // min x, max y, max x, min y
    vec2f offset;
    vec2f size;
    uint16 fileIndex;
    uint8 plane; // 0 .. 3
};

class Font
{
public:
    std::string debugId;

    Font();
    ~Font();
    void load(ResourceInfo &info, GpuFontSpec &spec,
        const std::string &debugId);

    Buffer fontData;
    std::shared_ptr<vts::FontHandle> fontHandle;
    std::vector<Glyph> glyphs;
    FT_Face face;
    hb_font_t *font;
    uint16 textureWidth;
    uint16 textureHeight;
    uint16 filesCount;
    uint8 size;
};

} } // namespace vts::renderer

#endif
