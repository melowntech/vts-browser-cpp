#include <cstdio>

#include <jpeglib.h>
#include <lodepng/lodepng.h>

#include "image.h"

namespace melown
{

namespace
{

void decodePng(const std::string &name, const Buffer &in, Buffer &out,
               uint32 &width, uint32 &height, uint32 &components)
{
    LodePNGState state;
    unsigned res = lodepng_inspect(&width, &height, &state,
                                   (unsigned char*)in.data, in.size);
    if (res != 0)
        throw imageDecodeException(
                std::string("failed to decode png image ") + name);
    components = state.info_png.color.colortype == LCT_RGBA ? 4 : 3;
    unsigned char *tmp = nullptr;
    res = lodepng_decode_memory(&tmp, &width, &height,
                                (unsigned char*)in.data, in.size,
                                components == 4 ? LCT_RGBA : LCT_RGB, 8);
    if (res != 0)
    {
        free(tmp);
        throw imageDecodeException(
                std::string("failed to decode png image ") + name);
    }
    out = Buffer(width * height * components);
    memcpy(out.data, tmp, out.size);
    free(tmp);
}

void decodeJpeg(const std::string &, const Buffer &in, Buffer &out,
                uint32 &width, uint32 &height, uint32 &components)
{
    jpeg_decompress_struct info;
    jpeg_error_mgr errmgr;
    info.err = jpeg_std_error(&errmgr);
    jpeg_create_decompress(&info);
    jpeg_mem_src(&info, (unsigned char*)in.data, in.size);
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
        ptr[0] = (unsigned char*)out.data
                + lineSize * (height - info.output_scanline - 1);
        jpeg_read_scanlines(&info, ptr, 1);
    }
    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);
}

} // namespace

void decodeImage(const std::string &name, const Buffer &in, Buffer &out,
                 uint32 &width, uint32 &height, uint32 &components)
{
    if (name.find(".png") != std::string::npos)
        decodePng(name, in, out, width, height, components);
    else
        decodeJpeg(name, in, out, width, height, components);
}

} // namespace melown
