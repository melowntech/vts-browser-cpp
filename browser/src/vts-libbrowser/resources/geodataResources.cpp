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

#include <dbglog/dbglog.hpp>

#include "../geodata.hpp"
#include "../fetchTask.hpp"
#include "../renderTasks.hpp"
#include "../map.hpp"
#include "../gpuResource.hpp"
#include "../utilities/json.hpp"

namespace vts
{

GeodataFeatures::GeodataFeatures(vts::MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

void GeodataFeatures::load()
{
    LOG(info2) << "Loading geodata features <" << name << ">";
    data = std::make_shared<const std::string>(
        std::move(fetch->reply.content.str()));
}

FetchTask::ResourceType GeodataFeatures::resourceType() const
{
    return FetchTask::ResourceType::GeodataFeatures;
}

GeodataStylesheet::GeodataStylesheet(MapImpl *map, const std::string &name) :
    Resource(map, name),
    dependenciesValidity(Validity::Indeterminate),
    dependenciesLoaded(false)
{
    priority = std::numeric_limits<float>::infinity();
}

void GeodataStylesheet::load()
{
    LOG(info2) << "Loading geodata stylesheet <" << name << ">";
    data = std::move(fetch->reply.content.str());
    dependenciesLoaded = false;
}

namespace
{

Validity merge(Validity a, Validity b)
{
    switch (a)
    {
    case Validity::Valid:
        return b;
    case Validity::Indeterminate:
        switch (b)
        {
        case Validity::Valid:
        case Validity::Indeterminate:
            return Validity::Indeterminate;
        case Validity::Invalid:
            return Validity::Invalid;
        }
        break;
    case Validity::Invalid:
        return Validity::Invalid;
    }
    return Validity::Invalid;
}

} // namespace

Validity GeodataStylesheet::dependencies()
{
    if (!dependenciesLoaded)
    {
        dependenciesValidity = Validity::Indeterminate;
        dependenciesLoaded = true;
        fonts.clear();
        bitmaps.clear();
        try
        {
            Json::Value s = stringToJson(data);
            for (const auto &n : s["fonts"].getMemberNames())
            {
                std::string p = s["fonts"][n].asString();
                p = convertPath(p, map->mapconfigPath);
                fonts[n] = map->getFont(p);
            }
            if (fonts.find("#default") == fonts.end())
            {
                std::string p = map->createOptions.geodataFontFallback;
                p = convertPath(p, map->mapconfigPath);
                fonts["#default"] = map->getFont(p);
            }
            /*
            for (const auto &n : s["bitmaps"].getMemberNames())
            {
                // todo bitmap may be an object with: url, filter, tiled
                std::string p = s["bitmaps"][n].asString();
                p = convertPath(p, map->mapconfigPath);
                bitmaps[n] = map->getTexture(p);
            }
            */
        }
        catch (std::exception &e)
        {
            LOG(err3) << "Failed parsing dependencies from stylesheet <"
                << name << ">, with error <"
                << e.what() << ">";
            dependenciesValidity = Validity::Invalid;
            return Validity::Invalid;
        }
        dependenciesValidity = Validity::Valid;
    }

    Validity valid = dependenciesValidity;
    for (const auto &it : fonts)
    {
        auto r = std::static_pointer_cast<Resource>(it.second);
        map->touchResource(r);
        valid = merge(valid, map->getResourceValidity(r));
    }
    for (const auto &it : bitmaps)
    {
        auto r = std::static_pointer_cast<Resource>(it.second);
        map->touchResource(r);
        valid = merge(valid, map->getResourceValidity(r));
    }
    return valid;
}

FetchTask::ResourceType GeodataStylesheet::resourceType() const
{
    return FetchTask::ResourceType::GeodataStylesheet;
}

GeodataTile::GeodataTile(MapImpl *map, const std::string &name)
    : Resource(map, name), lod(0)
{
    state = Resource::State::ready;

    // initialize aabb to universe
    {
        double di = std::numeric_limits<double>::infinity();
        vec3 vi(di, di, di);
        aabbPhys[0] = -vi;
        aabbPhys[1] = vi;
    }
}

FetchTask::ResourceType GeodataTile::resourceType() const
{
    return FetchTask::ResourceType::Undefined;
}

void GeodataTile::update(
    const std::shared_ptr<GeodataStylesheet> &s,
    const std::shared_ptr<const std::string> &f,
    const vec3 ab[2], uint32 l)
{
    switch ((Resource::State)state)
    {
    case Resource::State::initializing:
        state = Resource::State::errorFatal; // if left in initializing, it would attempt to download it
        UTILITY_FALLTHROUGH;
    case Resource::State::errorFatal: // allow reloading when sources change, even if it failed before
    case Resource::State::ready:
        if (style != s || features != f || lod != l
            || ab[0] != aabbPhys[0] || ab[1] != aabbPhys[1])
        {
            style = s;
            features = f;
            aabbPhys[0] = ab[0];
            aabbPhys[1] = ab[1];
            lod = l;
            state = Resource::State::downloading;
            map->resources.queGeodata.push(
                std::dynamic_pointer_cast<GeodataTile>(shared_from_this()));
            return;
        }
        break;
    default:
        // nothing
        break;
    }
}

GpuGeodataSpec::GpuGeodataSpec()
{
    matToRaw(identityMatrix4(), model);
}

GpuGeodataSpec::UnionData::UnionData()
{
    memset(this, 0, sizeof(*this));
}

GpuGeodataSpec::CommonData::CommonData()
{
    // zero all including any potential paddings
    //   this is important for binary comparisons of the entire struct
    memset(this, 0, sizeof(*this));

    stick.width = nan1();
    vecToRaw(vec4f(nan4().cast<float>()), visibilities);
    vecToRaw(vec3f(nan3().cast<float>()), zBufferOffset);
}

} // namespace vts
