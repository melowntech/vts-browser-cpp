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

#include <png.h>
#include <dbglog/dbglog.hpp>
#include "../include/vts-browser/buffer.hpp"

namespace vts
{

namespace
{

void pngErrFunc(png_structp, png_const_charp err)
{
    LOGTHROW(err1, std::runtime_error)
            << "failed to decode png image <" << err << ">";
}

struct pngInfoCtx
{
    png_structp png;
    png_infop info;
    bool writing;

    pngInfoCtx() : png(nullptr), info(nullptr), writing(false) {}
    ~pngInfoCtx()
    {
        if (info)
            png_destroy_info_struct(png, &info);
        if (png)
        {
            if (writing)
                png_destroy_write_struct(&png, nullptr);
            else
                png_destroy_read_struct(&png, nullptr, nullptr);
        }
    }
};

struct pngIoCtx
{
    Buffer &buf;
    uint32 off;
    pngIoCtx(Buffer &buf) : buf(buf), off(0) {}
};

void pngReadFunc(png_structp png, png_bytep buf, png_size_t siz)
{
    pngIoCtx *io = (pngIoCtx*)png_get_io_ptr(png);
    if (io->off + siz > io->buf.size())
        png_error(png, "png reading outside memory buffer");
    memcpy(buf, io->buf.data() + io->off, siz);
    io->off += siz;
}

void pngWriteFunc(png_structp png, png_bytep buf, png_size_t siz)
{
    pngIoCtx *io = (pngIoCtx*)png_get_io_ptr(png);
    io->buf.resize(io->off + siz);
    memcpy(io->buf.data() + io->off, buf, siz);
    io->off += siz;
}

void pngFlushFunc(png_structp)
{
    // do nothing
}

} // namespace

void decodePng(const Buffer &in, Buffer &out,
               uint32 &width, uint32 &height, uint32 &components)
{
    pngInfoCtx ctx;
    png_structp &png = ctx.png;
    png_infop &info = ctx.info;
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                 &pngErrFunc, nullptr);
    if (!png)
        LOGTHROW(err1, std::runtime_error)
                << "png image decoder init failed (png_structp)";
    pngIoCtx ioCtx(const_cast<Buffer&>(in));
    png_set_read_fn(png, &ioCtx, &pngReadFunc);
    info = png_create_info_struct(png);
    if (!info)
        LOGTHROW(err1, std::runtime_error)
            << "png image decoder init failed (png_infop)";
    png_read_info(png, info);
    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);

    png_set_expand(png);
    png_set_strip_16(png);
    png_set_palette_to_rgb(png);
    png_set_expand_gray_1_2_4_to_8(png);
    png_set_tRNS_to_alpha(png);

    png_read_update_info(png, info);
    png_byte colorType = png_get_color_type(png, info);

    switch (colorType)
    {
    case PNG_COLOR_TYPE_GRAY:
        components = 1;
        break;
    case PNG_COLOR_TYPE_GA:
        components = 2;
        break;
    case PNG_COLOR_TYPE_RGB:
        components = 3;
        break;
    case PNG_COLOR_TYPE_RGBA:
        components = 4;
        break;
    default:
        LOGTHROW(err1, std::runtime_error)
                << "png image has unsupported color type";
    }

    std::vector<png_bytep> rows;
    rows.resize(height);
    uint32 cols = width * components;
    assert(cols == png_get_rowbytes(png,info));
    out.allocate(height * cols);
    for (uint32 y = 0; y < height; y++)
        rows[y] = (png_bytep)out.data() + y * cols;
    png_read_image(png, rows.data());
}

void encodePng(const Buffer &in, Buffer &out,
               uint32 width, uint32 height, uint32 components)
{
    if (in.size() != width * height * components)
    {
        LOGTHROW(err2, std::runtime_error) << "Buffer with data for png "
                                              "encoding has invalid size";
    }

    pngInfoCtx ctx;
    ctx.writing = true;
    png_structp &png = ctx.png;
    png_infop &info = ctx.info;
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                  &pngErrFunc, nullptr);
    if (!png)
        LOGTHROW(err1, std::runtime_error)
                << "png encoder failed (png_structp)";
    pngIoCtx ioCtx(out);
    png_set_write_fn(png, &ioCtx, &pngWriteFunc, &pngFlushFunc);
    info = png_create_info_struct(png);
    if (!info)
        LOGTHROW(err1, std::runtime_error)
                << "png encoder failed (png_infop)";

    int colorType;
    switch (components)
    {
    case 1:
        colorType = PNG_COLOR_TYPE_GRAY;
        break;
    case 2:
        colorType = PNG_COLOR_TYPE_GA;
        break;
    case 3:
        colorType = PNG_COLOR_TYPE_RGB;
        break;
    case 4:
        colorType = PNG_COLOR_TYPE_RGBA;
        break;
    default:
        LOGTHROW(err1, std::logic_error)
                << "png encoder failed (unsupported color type)";
        throw;
    }

    png_set_IHDR(png, info, width, height, 8, colorType,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    std::vector<png_bytep> rows;
    rows.resize(height);
    uint32 cols = width * components;
    assert(cols == png_get_rowbytes(png,info));
    for (uint32 y = 0; y < height; y++)
        rows[y] = (png_bytep)in.data() + y * cols;
    png_write_image(png, rows.data());
    png_write_end(png, info);
}

} // namespace vts
