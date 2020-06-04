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

#include "../geodata.hpp"
#include "../fetchTask.hpp"
#include "../renderTasks.hpp"
#include "../map.hpp"
#include "../gpuResource.hpp"
#include "../utilities/json.hpp"

#include <dbglog/dbglog.hpp>

namespace vts
{

GeodataFeatures::GeodataFeatures(vts::MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

void GeodataFeatures::decode()
{
    LOG(info2) << "Decoding geodata features <" << name << ">";
    data = std::make_shared<const std::string>(fetch->reply.content.str());

#ifndef __EMSCRIPTEN__
    if (map->options.debugExtractRawResources)
    {
        static const std::string prefix = "extracted/";
        std::string b, c;
        std::string path = prefix
            + convertNameToFolderAndFile(this->name, b, c)
            + ".json";
        if (!boost::filesystem::exists(path))
        {
            boost::filesystem::create_directories(prefix + b);
            writeLocalFileBuffer(path, Buffer(*data));
        }
    }
#endif
}

FetchTask::ResourceType GeodataFeatures::resourceType() const
{
    return FetchTask::ResourceType::GeodataFeatures;
}

GeodataStylesheet::GeodataStylesheet(MapImpl *map, const std::string &name) :
    Resource(map, name)
{
    priority = inf1();
}

void GeodataStylesheet::decode()
{
    LOG(info2) << "Decoding geodata stylesheet <" << name << ">";
    data = fetch->reply.content.str();
    dependenciesLoaded = false;

#ifndef __EMSCRIPTEN__
    if (map->options.debugExtractRawResources)
    {
        static const std::string prefix = "extracted/";
        std::string b, c;
        std::string path = prefix
            + convertNameToFolderAndFile(this->name, b, c)
            + ".json";
        if (!boost::filesystem::exists(path))
        {
            boost::filesystem::create_directories(prefix + b);
            writeLocalFileBuffer(path, Buffer(data));
        }
    }
#endif
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

std::shared_ptr<GpuTexture> bitmapTexture(MapImpl *map, const std::string &v)
{
    std::string p = convertPath(v, map->mapconfigPath);
    return map->getTexture(p);
}

std::shared_ptr<GpuTexture> bitmapTexture(MapImpl *map, const Json::Value &v)
{
    if (v.isObject())
    {
        std::string url = v["url"].asString();
        std::string filter = v.isMember("filter")
            ? v["filter"].asString() : "linear";
        bool tiled = v.isMember("tiled")
            ? v["tiled"].asBool() : false;
        std::shared_ptr<GpuTexture> tex = bitmapTexture(map, url);
        if (filter == "nearest")
            tex->filterMode = GpuTextureSpec::FilterMode::Nearest;
        else if (filter == "linear")
            tex->filterMode = GpuTextureSpec::FilterMode::Linear;
        else if (filter == "trilinear")
            tex->filterMode = GpuTextureSpec::FilterMode::LinearMipmapLinear;
        else
            LOGTHROW(err2, std::runtime_error)
            << "Invalid style bitmap filter definition <"
            << filter << ">";
        tex->wrapMode = tiled ? GpuTextureSpec::WrapMode::Repeat
            : GpuTextureSpec::WrapMode::ClampToEdge;
        return tex;
    }
    else if (v.isString())
    {
        return bitmapTexture(map, v.asString());
    }
    else
    {
        LOGTHROW(err2, std::runtime_error)
            << "Invalid style bitmap definition <"
            << v.toStyledString() << ">";
        throw;
    }
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
            json = std::make_shared<const Json::Value>(stringToJson(data));
            auto &s = *json;
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
            for (const auto &n : s["bitmaps"].getMemberNames())
                bitmaps[n] = bitmapTexture(map, s["bitmaps"][n]);
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
    : Resource(map, name)
{
    state = Resource::State::ready;

    // initialize aabb to universe
    aabbPhys[0] = -inf3();
    aabbPhys[1] = inf3();
}

GeodataTile::~GeodataTile()
{
    for (auto &it : renders)
    {
        if (it.userData)
        {
            map->resources.queUpload.push(
                UploadData(it.userData, 0));
        }
    }
}

FetchTask::ResourceType GeodataTile::resourceType() const
{
    return FetchTask::ResourceType::Undefined;
}

void GeodataTile::update(
    const std::shared_ptr<GeodataStylesheet> &s,
    const std::shared_ptr<const std::string> &f,
    const std::shared_ptr<const Json::Value> &b,
    const vec3 ab[2], const TileId &tid)
{
    switch ((Resource::State)state)
    {
    case Resource::State::initializing:
        state = Resource::State::errorFatal; // if left in initializing, it would attempt to download it
        UTILITY_FALLTHROUGH;
    case Resource::State::errorFatal: // allow reloading when sources change, even if it failed before
    case Resource::State::ready:
        if (style != s || features != f || browserOptions != b
            || tileId != tid || ab[0] != aabbPhys[0] || ab[1] != aabbPhys[1])
        {
            style = s;
            features = f;
            browserOptions = b;
            aabbPhys[0] = ab[0];
            aabbPhys[1] = ab[1];
            tileId = tid;
            state = Resource::State::downloaded;
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

GpuGeodataSpec::GpuGeodataSpec() : type(GpuGeodataSpec::Type::Invalid)
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
    icon.scale = nan1();
    vecToRaw(vec4f(nan4().cast<float>()), visibilities);
    vecToRaw(vec3f(nan3().cast<float>()), zBufferOffset);
    featuresLimitPerPixelSquared = nan1();
    depthVisibilityThreshold = nan1();
    preventOverlap = true;
}

} // namespace vts
