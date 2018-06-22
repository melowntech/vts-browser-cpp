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

#include <thread>
#include <atomic>

#include "renderer.hpp"

#include <vts-libs/vts/atmosphereDensityTexture.hpp>

namespace vts { namespace renderer { namespace priv
{

namespace
{

bool areSame(const vts::MapCelestialBody &a, const vts::MapCelestialBody &b)
{
    return memcmp(&a.atmosphere, &b.atmosphere, sizeof(a.atmosphere)) == 0;
}

} // namespace

class AtmosphereImpl
{
public:
    MapCelestialBody body;
    GpuTextureSpec spec;
    std::shared_ptr<Texture> tex;
    std::shared_ptr<std::thread> thr;
    std::atomic<uint32> state;

    void threadEntry();
    Texture *validate(const MapCelestialBody &current);
};

void AtmosphereImpl::threadEntry()
{
    try
    {
        vts::setLogThreadName("atmosphere");
        vts::log(vts::LogLevel::info2, "Loading atmosphere density texture");

        double boundaryThickness, horizontalExponent, verticalExponent;
        AtmosphereDerivedAttributes(body, boundaryThickness,
            horizontalExponent, verticalExponent);

        double atmHeight = boundaryThickness / body.majorRadius;
        double atmRad = 1 + atmHeight;

        spec.buffer.free();
        spec.width = 512;
        spec.height = 512;
        spec.components = 4;

        std::string name;
        {
            std::stringstream ss;
            ss << std::setprecision(7) << std::fixed
               << "density-" << spec.width << "-" << spec.height << "-"
               << atmRad << "-" << verticalExponent << ".png";
            name = ss.str();
        }

        // try to load the texture from internal memory
        std::string fullName = std::string("data/textures/atmosphere/") + name;
        if (detail::existsInternalMemoryBuffer(fullName))
        {
            vts::log(vts::LogLevel::info2, "The atmosphere texture will "
                     "be loaded from internal memory");
            try
            {
                GpuTextureSpec sp(readInternalMemoryBuffer(fullName));
                if (spec.expectedSize() == sp.buffer.size())
                    std::swap(sp, spec);
            }
            catch (...)
            {
                // dont care
            }
        }

        // generate the texture anew
        if (spec.buffer.size() == 0)
        {
            vts::log(vts::LogLevel::info3,
                     "The atmosphere density texture will be generated anew");

            vtslibs::vts::generateAtmosphereTextureSpec gats;
            gats.width = spec.width;
            gats.height = spec.height;
            gats.thickness = atmHeight;
            gats.verticalCoefficient = verticalExponent;
            vtslibs::vts::generateAtmosphereTexture(gats);
            assert(gats.components == spec.components);
            assert(gats.width == spec.width && gats.height == spec.height);
            assert(gats.data.size() == spec.width * spec.height
                * spec.components);
            spec.buffer.resize(gats.data.size());
            memcpy(spec.buffer.data(), gats.data.data(), gats.data.size());
        }

        vts::log(vts::LogLevel::info2, "Finished atmosphere density texture");
        assert(state == 1);
        state = 2;
    }
    catch (const std::exception &e)
    {
        vts::log(LogLevel::err3, e.what());
    }
    catch (...)
    {
        vts::log(LogLevel::err4, "Unknown exception in atmosphere "
                                 "density texture thread");
    }
}

AtmosphereDensity::AtmosphereDensity()
{
    impl = std::make_shared<AtmosphereImpl>();
}

AtmosphereDensity::~AtmosphereDensity()
{}

void AtmosphereDensity::initialize()
{}

void AtmosphereDensity::finalize()
{
    impl->tex.reset();
}

Texture *AtmosphereDensity::validate(const MapCelestialBody &current)
{
    return impl->validate(current);
}

Texture *AtmosphereImpl::validate(const MapCelestialBody &current)
{
    switch ((uint32)state)
    {
    case 0: // initializing
        assert(!thr);
        tex.reset();
        if (current.atmosphere.thickness > 0)
        {
            body = current;
            state = 1;
            thr = std::make_shared<std::thread>(
                    std::bind(&AtmosphereImpl::threadEntry, this), this);
        }
        return nullptr;
    case 1: // computing
        return nullptr;
    case 2: // prepared
        thr->join();
        thr.reset();
        tex = std::make_shared<Texture>();
        tex->load(spec);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        spec.buffer.free();
        state = 3;
        return tex.get();
    case 3: // ready
        if (areSame(current, body))
            return tex.get();
        state = 0;
        return nullptr;
    default:
        assert(false);
        throw;
    }
}

} } } // namespace vts renderer priv

