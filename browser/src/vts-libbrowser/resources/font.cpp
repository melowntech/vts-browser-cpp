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

#include "../gpuResource.hpp"
#include "../fetchTask.hpp"
#include "../map.hpp"

#include <dbglog/dbglog.hpp>
#include <sstream>

namespace vts
{

FontHandle::~FontHandle()
{}

GpuFont::GpuFont(MapImpl *map, const std::string &name) :
    Resource(map, name)
{
    priority = std::numeric_limits<float>::infinity();
}

void GpuFont::decode()
{
    LOG(info2) << "Decoding font <" << name << ">";

#ifndef __EMSCRIPTEN__
    if (map->options.debugExtractRawResources)
    {
        static const std::string prefix = "extracted/";
        std::string b, c;
        std::string path = prefix
            + convertNameToFolderAndFile(this->name, b, c)
            + ".fnt";
        if (!boost::filesystem::exists(path))
        {
            boost::filesystem::create_directories(prefix + b);
            writeLocalFileBuffer(path, fetch->reply.content);
        }
    }
#endif

    std::shared_ptr<GpuFontSpec> spec = std::make_shared<GpuFontSpec>();
    spec->data = std::move(fetch->reply.content);
    spec->handle = std::dynamic_pointer_cast<FontHandle>(shared_from_this());
    decodeData = std::static_pointer_cast<void>(spec);
}

void GpuFont::upload()
{
    LOG(info2) << "Uploading font <" << name << ">";
    auto spec = std::static_pointer_cast<GpuFontSpec>(decodeData);
    map->callbacks.loadFont(info, *spec, name);
}

std::shared_ptr<void> GpuFont::requestTexture(uint32 index)
{
    if (texturePlanes.size() <= index)
        texturePlanes.resize(index + 1);
    auto t = texturePlanes[index];
    if (!t)
    {
        // David, who made initial geodata (and fonts) implementation,
        //   had a strong aesthetic feelings about how to name the textures:
        //   something.fnt is THE header
        //   something.fnt0 and something.fnt1 are skipped
        //   something.fnt2 is the first texture (at index 0)
        // We honor the offset
        //   not because it would not work
        //   but because we understand his reasoning, right?
        std::stringstream ss;
        ss << this->name << (index + 2);
        std::string n = ss.str();
        texturePlanes[index] = t = map->getTexture(n);
    }
    else
        map->touchResource(t);
    if (*t)
        return t->getUserData();
    return nullptr;
}

FetchTask::ResourceType GpuFont::resourceType() const
{
    return FetchTask::ResourceType::Font;
}

} // namespace vts
