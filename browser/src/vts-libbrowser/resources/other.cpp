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
#include "../metaTile.hpp"
#include "../fetchTask.hpp"
#include "../mapConfig.hpp"
#include "../tilesetMapping.hpp"

#include <dbglog/dbglog.hpp>

namespace vts
{

BoundMetaTile::BoundMetaTile(MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

void BoundMetaTile::decode()
{
    Buffer buffer = std::move(fetch->reply.content);
    GpuTextureSpec spec;
    decodeImage(buffer, spec.buffer,
                spec.width, spec.height, spec.components);
    if (spec.buffer.size() != sizeof(flags))
        LOGTHROW(err1, std::runtime_error)
                << "bound meta tile has invalid resolution";
    memcpy(flags, spec.buffer.data(), spec.buffer.size());
    info.ramMemoryCost += sizeof(*this);
    info.ramMemoryCost += spec.buffer.size();
}

FetchTask::ResourceType BoundMetaTile::resourceType() const
{
    return FetchTask::ResourceType::BoundMetaTile;
}

ExternalBoundLayer::ExternalBoundLayer(MapImpl *map, const std::string &name)
    : Resource(map, name)
{
    priority = std::numeric_limits<float>::infinity();
}

void ExternalBoundLayer::decode()
{
    detail::BufferStream w(fetch->reply.content);
    *(vtslibs::registry::BoundLayer*)this
            = vtslibs::registry::loadBoundLayer(w, name);
}

FetchTask::ResourceType ExternalBoundLayer::resourceType() const
{
    return FetchTask::ResourceType::BoundLayerConfig;
}

ExternalFreeLayer::ExternalFreeLayer(MapImpl *map, const std::string &name)
    : Resource(map, name)
{
    priority = std::numeric_limits<float>::infinity();
}

void ExternalFreeLayer::decode()
{
    detail::BufferStream w(fetch->reply.content);
    *(vtslibs::registry::FreeLayer*)this
            = vtslibs::registry::loadFreeLayer(w, name);
}

FetchTask::ResourceType ExternalFreeLayer::resourceType() const
{
    return FetchTask::ResourceType::FreeLayerConfig;
}

TilesetMapping::TilesetMapping(MapImpl *map, const std::string &name) :
    Resource(map, name)
{
    priority = std::numeric_limits<float>::infinity();
}

void TilesetMapping::decode()
{
    dataRaw = vtslibs::vts::deserializeTsMap(fetch->reply.content.str());
}

FetchTask::ResourceType TilesetMapping::resourceType() const
{
    return FetchTask::ResourceType::TilesetMappingConfig;
}

} // namespace vts
