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

#include "renderer.hpp"
#include "font.hpp"
#include "utility/binaryio.hpp"

namespace bin = utility::binaryio;

namespace
{

FT_Library ftLibrary;

class FtInitializer
{
public:
    FtInitializer()
    {
        if (FT_Init_FreeType(&ftLibrary))
            throw std::runtime_error("Failed to initialize FreeType");
    }
} ftInitializerInstance;

void swapEndian(uint16 &v)
{
    v = (v >> 8) | (v << 8);
}

void swapEndian(uint32 &v)
{
    v = (v >> 24)
        | (v << 24)
        | (((v >> 16) & 255) << 8)
        | (((v >> 8) & 255) << 16);
}

uint32 findCustomTablesOffset(const vts::Buffer &buf)
{
    vts::detail::BufferStream w(buf);
    uint16 dummy16;
    uint32 dummy32;
    // sfnt_version
    bin::read(w, dummy32);
    uint16 numTables = 0;
    bin::read(w, numTables);
    swapEndian(numTables);
    // searchRange, entrySelector, rangeShift
    bin::read(w, dummy16);
    bin::read(w, dummy16);
    bin::read(w, dummy16);
    uint32 result = 0;
    for (uint32 tableIndex = 0; tableIndex < numTables; tableIndex++)
    {
        // tag, checksum
        bin::read(w, dummy32);
        bin::read(w, dummy32);
        uint32 to = 0, ts = 0;
        bin::read(w, to);
        bin::read(w, ts);
        swapEndian(to);
        swapEndian(ts);
        result = std::max(result, to + ts);
    }
    return result;
}

} // namespace

namespace vts { namespace renderer
{

Font::Font() : face(nullptr), font(nullptr), filesCount(0)
{}

Font::~Font()
{
    hb_font_destroy(font);
    FT_Done_Face(face);
}

void Font::load(ResourceInfo &info, GpuFontSpec &spec)
{
    fontHandle = spec.handle;

    if (FT_New_Memory_Face(ftLibrary,
        (FT_Byte*)spec.data.data(), spec.data.size(),
        0, &face))
        throw std::runtime_error("Failed loading the font with FreeType");

    vts::detail::BufferStream w(spec.data);
    w.ignore(findCustomTablesOffset(spec.data));
    {
        uint8 version;
        bin::read(w, version);
    }
    bin::read(w, textureWidth);
    bin::read(w, textureHeight);
    swapEndian(textureWidth);
    swapEndian(textureHeight);
    uint8 size;
    bin::read(w, size);
    {
        uint8 flags;
        bin::read(w, flags);
    }

    if (FT_Set_Char_Size(face, size * 64, size * 64, 0, 0))
        throw std::runtime_error("FT_Set_Char_Size");
    font = hb_ft_font_create(face, nullptr);

    // generate glyphs table
    {
        glyphs.resize(face->num_glyphs);
        for (uint32 i = 0; i < face->num_glyphs; i++)
        {
            // w 6bit | h 6bit | sx sign 1bit | abs sx 6bit | sy sign 1bit | abs sy 6bit | plane 2bit
            uint32 v1;
            bin::read(w, v1);
            swapEndian(v1);
            uint16 v2;
            bin::read(w, v2);
            swapEndian(v2);
            Glyph &g = glyphs[i];
            g.plane = v1 & 3;
            uint8 gw = (v1 >> 22) & 63;
            uint8 gh = (v1 >> 16) & 63;
            uint8 gx = (v2 >> 8) & 255;
            uint8 gy = v2 & 255;
            g.uvs = vec4f(gx, gy, gx + gw, gy + gh) / (textureWidth - 1);
        }
    }

    // find glyphs file indices
    {
        bin::read(w, filesCount);
        swapEndian(filesCount);
        uint16 last = 0, curr = 0;
        for (uint16 i = 0; i < filesCount; i++)
        {
            bin::read(w, curr);
            swapEndian(curr);
            for (uint16 j = last; j < curr; j++)
                glyphs[j].fileIndex = i;
            last = curr;
        }
    }
}

void Font::load(GpuFontSpec &spec)
{
    ResourceInfo info;
    load(info, spec);
}

void Renderer::loadFont(ResourceInfo &info, GpuFontSpec &spec)
{
    auto r = std::make_shared<Font>();
    r->load(info, spec);
    info.userData = r;
}

} } // namespace vts renderer

