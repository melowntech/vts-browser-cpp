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
#include "../utilities/json.hpp"

namespace vts
{

GeodataFeatures::GeodataFeatures(vts::MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::GeodataFeatures)
{}

void GeodataFeatures::load()
{
    LOG(info2) << "Loading geodata features <" << name << ">";
    data = reply.content.str();
}

GeodataStylesheet::GeodataStylesheet(MapImpl *map, const std::string &name) :
    Resource(map, name, FetchTask::ResourceType::GeodataStylesheet)
{
    priority = std::numeric_limits<float>::infinity();
}

void GeodataStylesheet::load()
{
    LOG(info2) << "Loading geodata stylesheet <" << name << ">";
    data = reply.content.str();
}

GpuGeodata::GpuGeodata(MapImpl *map, const std::string &name)
    : Resource(map, name, FetchTask::ResourceType::General)
{
    state = Resource::State::ready;
}

void GpuGeodata::load()
{
    LOG(info2) << "Loading gpu-geodata <" << name << ">";
    renders.clear();

    if (style.empty() || features.empty())
    {
        renders.emplace_back();
        return;
    }

    Json::Value styleRoot = stringToJson(style);
    Json::Value featuresRoot = stringToJson(features);

    // todo
    // parse json data
    // parse json style
    // apply expression evaluations
    // generate draw commands

    renders.emplace_back();
}

void GpuGeodata::update(const std::string &s, const std::string &f)
{
    switch ((Resource::State)state)
    {
    case Resource::State::initializing:
        state = Resource::State::ready;
        // no break
    case Resource::State::ready:
        if (style != s || features != f)
        {
            style = s;
            features = f;
            state = Resource::State::downloaded;
        }
        break;
    default:
        // nothing
        break;
    }
}

} // namespace vts
