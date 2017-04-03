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
    lodepng_state_init(&state);
    unsigned res = lodepng_inspect(&width, &height, &state,
                                   (unsigned char*)in.data(), in.size());
    if (res != 0)
        throw std::runtime_error(
                std::string("failed to decode png image ") + name);
    components = lodepng_get_channels(&state.info_png.color);
    unsigned char *tmp = nullptr;
    res = lodepng_decode_memory(&tmp, &width, &height,
                                (unsigned char*)in.data(), in.size(),
                                state.info_png.color.colortype, 8);
    if (res != 0)
    {
        free(tmp);
        throw std::runtime_error(
                std::string("failed to decode png image ") + name);
    }
    out = Buffer(width * height * components);
    memcpy(out.data(), tmp, out.size());
    free(tmp);
}

void jpegErrFunc(j_common_ptr cinfo)
{
    char jpegLastErrorMsg[JMSG_LENGTH_MAX];
    (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
    throw std::runtime_error(std::string("failed to decode jpeg image: ")
                               + jpegLastErrorMsg);
}

void decodeJpeg(const std::string &, const Buffer &in, Buffer &out,
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

void decodeImage(const std::string &name, const Buffer &in, Buffer &out,
                 uint32 &width, uint32 &height, uint32 &components)
{
    if (in.size() < 8)
        throw std::runtime_error("invalid image data");
    static const unsigned char pngSignature[]
            = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if (memcmp(in.data(), pngSignature, 8) == 0)
        decodePng(name, in, out, width, height, components);
    else
        decodeJpeg(name, in, out, width, height, components);
}

} // namespace melown
