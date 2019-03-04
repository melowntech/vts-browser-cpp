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
#include <vts-libs/vts/atmospheredensitytexture.hpp>

#include "../include/vts-browser/log.hpp"
#include "../include/vts-browser/celestial.hpp"

#include "../image/image.hpp"
#include "../gpuResource.hpp"
#include "../fetchTask.hpp"
#include "../map.hpp"
#include "../mapConfig.hpp"

namespace vts
{

namespace
{

void generateAtmosphereTexture(
    const std::shared_ptr<GpuAtmosphereDensityTexture> &tex,
    const MapCelestialBody &body)
{
    double boundaryThickness, horizontalExponent, verticalExponent;
    atmosphereDerivedAttributes(body, boundaryThickness,
        horizontalExponent, verticalExponent);

    // generate the texture anew
    vtslibs::vts::AtmosphereTextureSpec gats;
    gats.thickness = boundaryThickness / body.majorRadius;
    gats.verticalCoefficient = verticalExponent;
    auto res = vtslibs::vts::generateAtmosphereTexture(gats,
            vtslibs::vts::AtmosphereTexture::Format::gray3);

    // store the result
    Buffer tmp(res.data.size());
    memcpy(tmp.data(), res.data.data(), res.data.size());
    encodePng(tmp, tex->fetch->reply.content,
        res.size.width, res.size.height, res.components);

    // mark the texture ready
    {
        tex->info.ramMemoryCost = tex->fetch->reply.content.size();
        tex->state = Resource::State::downloaded;
        tex->map->resources.queUpload.push(tex);
    }

    // (deferred) write to cache
    tex->map->resources.queCacheWrite.push(tex->fetch.get());
}

void gray3ToRgb(GpuTextureSpec &spec)
{
    if (spec.components != 1)
    {
        LOGTHROW(err2, std::runtime_error)
            << "Atmosphere density texture"
            " has <" << spec.components << "> components,"
            " but grayscale is expected.";
    }
    spec.components = 3;
    spec.height /= 3;

    Buffer orig = std::move(spec.buffer);
    const char *r = orig.data() + (spec.width * spec.height) * 0;
    const char *g = orig.data() + (spec.width * spec.height) * 1;
    const char *b = orig.data() + (spec.width * spec.height) * 2;

    spec.buffer.allocate(spec.width * spec.height * spec.components);
    char *res = spec.buffer.data();
    for (uint32 y = 0; y < spec.height; y++)
    {
        for (uint32 x = 0; x < spec.width; x++)
        {
            res[(y * spec.width + x) * spec.components + 0]
                = r[y * spec.width + x];
            res[(y * spec.width + x) * spec.components + 1]
                = g[y * spec.width + x];
            res[(y * spec.width + x) * spec.components + 2]
                = b[y * spec.width + x];
        }
    }
}

} // namespace

GpuAtmosphereDensityTexture::GpuAtmosphereDensityTexture(MapImpl *map,
    const std::string &name) : GpuTexture(map, name)
{
    filterMode = GpuTextureSpec::FilterMode::Nearest;
    wrapMode = GpuTextureSpec::WrapMode::ClampToEdge;
}

void GpuAtmosphereDensityTexture::load()
{
    LOG(info2) << "Loading (gpu) texture <" << name << ">";
    GpuTextureSpec spec(fetch->reply.content);
    spec.filterMode = filterMode;
    spec.wrapMode = wrapMode;
    gray3ToRgb(spec);
    map->callbacks.loadTexture(info, spec, name);
    info.ramMemoryCost += sizeof(*this);
}

void MapImpl::resourcesAtmosphereGeneratorEntry()
{
    setLogThreadName("atmosphere generator");
    while (!resources.queAtmosphere.stopped())
    {
        std::weak_ptr<GpuAtmosphereDensityTexture> w;
        resources.queAtmosphere.waitPop(w);
        std::shared_ptr<GpuAtmosphereDensityTexture> r = w.lock();
        if (!r)
            continue;
        try
        {
            generateAtmosphereTexture(r, body);
        }
        catch (const std::exception &)
        {
            r->state = Resource::State::errorFatal;
        }
    }
}

void MapImpl::updateAtmosphereDensity()
{
    const std::shared_ptr<GpuAtmosphereDensityTexture> &tex
        = mapconfig->atmosphereDensityTexture;
    assert(tex);
    touchResource(tex);
    switch ((Resource::State)tex->state)
    {
    case Resource::State::errorRetry:
    case Resource::State::availFail:
        break;
    default:
        return;
    }

    tex->state = Resource::State::downloading;
    resources.queAtmosphere.push(tex);
}

}
