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

#include "map.hpp"
#include "image/image.hpp"
#include "include/vts-browser/log.hpp"

#include <vts-libs/vts/atmosphereDensityTexture.hpp>

namespace vts
{

namespace
{

void generateAtmosphereTexture(const std::shared_ptr<GpuTexture> &tex,
    const MapCelestialBody &body)
{
    double boundaryThickness, horizontalExponent, verticalExponent;
    atmosphereDerivedAttributes(body, boundaryThickness,
        horizontalExponent, verticalExponent);

    // generate the texture anew
    vtslibs::vts::generateAtmosphereTextureSpec gats;
    gats.thickness = boundaryThickness / body.majorRadius;
    gats.verticalCoefficient = verticalExponent;
    vtslibs::vts::generateAtmosphereTexture(gats);

    // store the result
    Buffer tmp(gats.data.size());
    memcpy(tmp.data(), gats.data.data(), gats.data.size());
    encodePng(tmp, tex->fetch->reply.content,
        gats.width, gats.height, gats.components);

    // mark the texture ready
    {
        tex->info.ramMemoryCost = tex->fetch->reply.content.size();
        tex->state = Resource::State::downloaded;
        tex->map->resources.queUpload.push(tex);
    }

    // (deferred) write to cache
    tex->map->resources.queCacheWrite.push(tex->fetch.get());
}

} // namespace

void MapImpl::resourcesAtmosphereGeneratorEntry()
{
    setLogThreadName("atmosphere generator");
    while (!resources.queAtmosphere.stopped())
    {
        std::weak_ptr<GpuTexture> w;
        resources.queAtmosphere.waitPop(w);
        std::shared_ptr<GpuTexture> r = w.lock();
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
    const std::shared_ptr<GpuTexture> tex
        = mapConfig->atmosphereDensityTexture;
    assert(tex);
    touchResource(tex);
    switch (tex->state)
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
