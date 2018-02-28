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

#include <vts-libs/vts/meshio.hpp>

#include "map.hpp"
#include "image.hpp"

namespace vts
{

MetaTile::MetaTile(vts::MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::MetaTile),
    vtslibs::vts::MetaTile(vtslibs::vts::TileId(), 0)
{}

void MetaTile::load()
{
    LOG(info2) << "Loading meta tile <" << name << ">";
    detail::Wrapper w(reply.content);
    *(vtslibs::vts::MetaTile*)this
            = vtslibs::vts::loadMetaTile(w, 5, name);
    info.ramMemoryCost += sizeof(*this);
    uint32 side = 1 << 5;
    info.ramMemoryCost += side * side * sizeof(vtslibs::vts::MetaNode);
}

NavTile::NavTile(MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::NavTile)
{}

void NavTile::load()
{
    LOG(info2) << "Loading navigation tile <" << name << ">";
    GpuTextureSpec spec;
    decodeImage(reply.content, spec.buffer,
                spec.width, spec.height, spec.components);
    if (spec.width != 256 || spec.height != 256 || spec.components != 1)
        LOGTHROW(err1, std::runtime_error) << "invalid navtile image";
    data.resize(256 * 256);
    memcpy(data.data(), spec.buffer.data(), 256 * 256);
    info.ramMemoryCost += sizeof(*this);
    info.ramMemoryCost += data.size();
}

vec2 NavTile::sds2px(const vec2 &point, const math::Extents2 &extents)
{
    return vecFromUblas<vec2>(vtslibs::vts::NavTile::sds2px(
                                  vecToUblas<math::Point2>(point), extents));
}

BoundMetaTile::BoundMetaTile(MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::BoundMetaTile)
{}

void BoundMetaTile::load()
{
    LOG(info2) << "Loading bound meta tile <" << name << ">";
    Buffer buffer = std::move(reply.content);
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

} // namespace vts
