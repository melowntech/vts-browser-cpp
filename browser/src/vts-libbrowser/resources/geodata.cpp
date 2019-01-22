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

#include "../map.hpp"
//#include "../utilities/json.hpp"

namespace vts
{

GeodataFeatures::GeodataFeatures(vts::MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

void GeodataFeatures::load()
{
    LOG(info2) << "Loading geodata features <" << name << ">";
    data = std::move(fetch->reply.content.str());
}

FetchTask::ResourceType GeodataFeatures::resourceType() const
{
    return FetchTask::ResourceType::GeodataFeatures;
}

GeodataStylesheet::GeodataStylesheet(MapImpl *map, const std::string &name) :
    Resource(map, name)
{
    priority = std::numeric_limits<float>::infinity();
}

void GeodataStylesheet::load()
{
    LOG(info2) << "Loading geodata stylesheet <" << name << ">";
    data = std::move(fetch->reply.content.str());
}

FetchTask::ResourceType GeodataStylesheet::resourceType() const
{
    return FetchTask::ResourceType::GeodataStylesheet;
}

GpuGeodata::GpuGeodata(MapImpl *map, const std::string &name)
    : Resource(map, name), lod(0)
{
    state = Resource::State::ready;
}

FetchTask::ResourceType GpuGeodata::resourceType() const
{
    return FetchTask::ResourceType::Undefined;
}

bool GpuGeodata::update(const std::string &s, const std::string &f, uint32 l)
{
    switch ((Resource::State)state)
    {
    case Resource::State::initializing:
        state = Resource::State::ready; // if left in initializing, it would attempt to download it
        UTILITY_FALLTHROUGH;
    case Resource::State::errorFatal: // allow reloading when sources change, even if it failed before
    case Resource::State::ready:
        if (style != s || features != f || lod != l)
        {
            style = s;
            features = f;
            lod = l;
            state = Resource::State::downloaded;
            return true;
        }
        break;
    default:
        // nothing
        break;
    }
    return false;
}

} // namespace vts
