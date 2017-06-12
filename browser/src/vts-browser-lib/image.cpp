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

#include <cstdio>

#include <jpeglib.h>
#include <lodepng/lodepng.h>
#include <dbglog/dbglog.hpp>

#include "image.hpp"

namespace vts
{

namespace
{

void decodePng(const Buffer &in, Buffer &out,
               uint32 &width, uint32 &height, uint32 &components)
{
    LodePNGState state;
    lodepng_state_init(&state);
    unsigned res = lodepng_inspect(&width, &height, &state,
                                   (unsigned char*)in.data(), in.size());
    if (res != 0)
        LOGTHROW(err1, std::runtime_error) << "failed to decode png image";
    components = lodepng_get_channels(&state.info_png.color);
    unsigned char *tmp = nullptr;
    res = lodepng_decode_memory(&tmp, &width, &height,
                                (unsigned char*)in.data(), in.size(),
                                state.info_png.color.colortype, 8);
    if (res != 0)
    {
        free(tmp);
        LOGTHROW(err1, std::runtime_error) << "failed to decode png image";
    }
    out = Buffer(width * height * components);
    memcpy(out.data(), tmp, out.size());
    free(tmp);
}

void jpegErrFunc(j_common_ptr cinfo)
{
    char jpegLastErrorMsg[JMSG_LENGTH_MAX];
    (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
    LOGTHROW(err1, std::runtime_error)
            << "failed to decode jpeg image: "
            << jpegLastErrorMsg;
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

} // namespace

void decodeImage(const Buffer &in, Buffer &out,
                 uint32 &width, uint32 &height, uint32 &components)
{
    if (in.size() < 8)
        LOGTHROW(err1, std::runtime_error) << "invalid image data";
    static const unsigned char pngSignature[]
            = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if (memcmp(in.data(), pngSignature, 8) == 0)
        decodePng(in, out, width, height, components);
    else
        decodeJpeg(in, out, width, height, components);
}

} // namespace vts
