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

#include "../image/image.hpp"
#include "../gpuResource.hpp"
#include "../fetchTask.hpp"
#include "../map.hpp"

#include <dbglog/dbglog.hpp>

namespace vts
{

GpuTextureSpec::GpuTextureSpec()
    : width(0), height(0), components(0),
    type(GpuTypeEnum::UnsignedByte), internalFormat(0),
    filterMode(FilterMode::NearestMipmapLinear), wrapMode(WrapMode::Repeat)
{}

GpuTextureSpec::GpuTextureSpec(const Buffer &buffer) : GpuTextureSpec()
{
    decodeImage(buffer, this->buffer, width, height, components);
}

void GpuTextureSpec::verticalFlip()
{
    uint32 lineSize = width * components;
    Buffer tmp(lineSize);
    for (uint32 y = 0; y < height / 2; y++)
    {
        char *a = buffer.data() + y * lineSize;
        char *b = buffer.data()
                + (height - y - 1) * lineSize;
        memcpy(tmp.data(), a, lineSize);
        memcpy(a, b, lineSize);
        memcpy(b, tmp.data(), lineSize);
    }
}

uint32 GpuTextureSpec::expectedSize() const
{
    return width * height * components * gpuTypeSize(type);
}

Buffer GpuTextureSpec::encodePng() const
{
    if (type != GpuTypeEnum::UnsignedByte)
    {
        LOGTHROW(err2, std::runtime_error) << "Unsigned byte is the only "
                                    "supported image type for png encode.";
    }
    Buffer out;
    vts::encodePng(buffer, out, width, height, components);
    return out;
}

GpuTexture::GpuTexture(MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

void GpuTexture::decode()
{
    LOG(info1) << "Decoding texture <" << name << ">";
    std::shared_ptr<GpuTextureSpec> spec
        = std::make_shared<GpuTextureSpec>(fetch->reply.content);
    this->width = spec->width;
    this->height = spec->height;
    spec->filterMode = filterMode;
    spec->wrapMode = wrapMode;

#ifndef __EMSCRIPTEN__
    if (map->options.debugExtractRawResources)
    {
        static const std::string prefix = "extracted/";
        std::string b, c;
        std::string path = prefix
                + convertNameToFolderAndFile(this->name, b, c)
                + ".png";
        if (!boost::filesystem::exists(path))
        {
            boost::filesystem::create_directories(prefix + b);
            Buffer out;
            encodePng(spec->buffer, out,
                spec->width, spec->height, spec->components);
            writeLocalFileBuffer(path, out);
        }
    }
#endif

    spec->verticalFlip();
    decodeData = std::static_pointer_cast<void>(spec);
}

void GpuTexture::upload()
{
    LOG(info2) << "Uploading texture <" << name << ">";
    auto spec = std::static_pointer_cast<GpuTextureSpec>(decodeData);
    map->callbacks.loadTexture(info, *spec, name);
    info.ramMemoryCost += sizeof(*this);
}

FetchTask::ResourceType GpuTexture::resourceType() const
{
    return FetchTask::ResourceType::Texture;
}

} // namespace vts
