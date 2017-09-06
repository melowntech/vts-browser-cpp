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
#include <jpeglib.h>
#include <dbglog/dbglog.hpp>
#include "../include/vts-browser/buffer.hpp"

namespace vts
{

void pngErrFunc(png_structp, png_const_charp err)
{
    LOGTHROW(err1, std::runtime_error)
            << "failed to decode png image <" << err << ">";
}

struct pngIoCtx
{
    const Buffer &in;
    uint32 off;

    pngIoCtx(const Buffer &in) : in(in), off(0)
    {}
};

void pngRwFunc(png_structp png, png_bytep buf, png_size_t siz)
{
    pngIoCtx *io = (pngIoCtx*)png_get_io_ptr(png);
    if (io->off + siz > io->in.size())
        png_error(png, "png reading outside memory");
    memcpy(buf, io->in.data() + io->off, siz);
    io->off += siz;
}

void decodePng(const Buffer &in, Buffer &out,
               uint32 &width, uint32 &height, uint32 &components)
{
    png_structp png = nullptr;
    png_infop info = nullptr;
    try
    {
        png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                               nullptr, &pngErrFunc, nullptr);
        pngIoCtx ioCtx(in);
        png_set_read_fn(png, &ioCtx, &pngRwFunc);
        if(!png)
        {
            LOGTHROW(err1, std::runtime_error)
                    << "png image decoder init failed (png_structp)";
        }
        info = png_create_info_struct(png);
        if (!info)
        {
            LOGTHROW(err1, std::runtime_error)
                << "png image decoder init failed (png_infop)";
        }
        png_read_info(png, info);
        width = png_get_image_width(png, info);
        height = png_get_image_height(png, info);
        png_byte colorType = png_get_color_type(png, info);
        png_byte bitDepth  = png_get_bit_depth(png, info);

        if(bitDepth == 16)
          png_set_strip_16(png);
        if(colorType == PNG_COLOR_TYPE_PALETTE)
          png_set_palette_to_rgb(png);
        if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
          png_set_expand_gray_1_2_4_to_8(png);
        if(png_get_valid(png, info, PNG_INFO_tRNS))
          png_set_tRNS_to_alpha(png);

        png_read_update_info(png, info);
        colorType = png_get_color_type(png, info);
        bitDepth  = png_get_bit_depth(png, info);
        assert(bitDepth == 8);
        switch (colorType)
        {
        case PNG_COLOR_TYPE_GRAY:
            components = 1;
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
    catch(...)
    {
        if (info)
            png_destroy_info_struct(png, &info);
        if (png)
            png_destroy_read_struct(&png, nullptr, nullptr);
        throw;
    }
    png_destroy_info_struct(png, &info);
    png_destroy_read_struct(&png, nullptr, nullptr);
}

void jpegErrFunc(j_common_ptr cinfo)
{
    char jpegLastErrorMsg[JMSG_LENGTH_MAX];
    (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
    LOGTHROW(err1, std::runtime_error)
            << "failed to decode jpeg image <"
            << jpegLastErrorMsg << ">";
}

void decodeJpeg(const Buffer &in, Buffer &out,
                uint32 &width, uint32 &height, uint32 &components)
{
    jpeg_decompress_struct info;
    jpeg_error_mgr errmgr;
    info.err = jpeg_std_error(&errmgr);
    errmgr.error_exit = &jpegErrFunc;
    try
    {
        jpeg_create_decompress(&info);
        jpeg_mem_src(&info, (unsigned char*)in.data(), in.size());
        jpeg_read_header(&info, TRUE);
        jpeg_start_decompress(&info);
        width = info.output_width;
        height = info.output_height;
        components = info.num_components;
        uint32 lineSize = components * width;
        out = Buffer(lineSize * height);
        while (info.output_scanline < info.output_height)
        {
            unsigned char *ptr[1];
            ptr[0] = (unsigned char*)out.data()
                    + lineSize * info.output_scanline;
            jpeg_read_scanlines(&info, ptr, 1);
        }
        jpeg_finish_decompress(&info);
        jpeg_destroy_decompress(&info);
    }
    catch (...)
    {
        jpeg_destroy_decompress(&info);
        throw;
    }
}

} // namespace vts
