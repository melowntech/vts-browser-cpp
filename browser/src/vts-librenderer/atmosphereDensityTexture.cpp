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

namespace vts { namespace renderer { namespace priv
{

namespace
{

bool areSame(const vts::MapCelestialBody &a, const vts::MapCelestialBody &b)
{
    return a.majorRadius == b.majorRadius
        && a.minorRadius == b.minorRadius
        && a.atmosphere.thickness == b.atmosphere.thickness
        && a.atmosphere.horizontalExponent == b.atmosphere.horizontalExponent
        && a.atmosphere.verticalExponent == b.atmosphere.verticalExponent;
}

void encodeFloat(double v, unsigned char *target)
{
    assert(v >= 0 && v < 1);
    vec4 enc = vec4(1.0, 256.0, 256.0*256.0, 256.0*256.0*256.0) * v;
    for (int i = 0; i < 4; i++)
        enc[i] -= std::floor(enc[i]); // frac
    vec4 tmp;
    for (int i = 0; i < 3; i++)
        tmp[i] = enc[i + 1] / 256.0; // shift
    tmp[3] = 0;
    enc -= tmp; // subtract
    for (int i = 0; i < 4; i++)
        target[i] = enc[i] * 256.0;
}

double sqr(double a)
{
    return a * a;
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

        double atmHeight = body.atmosphere.thickness / body.majorRadius;
        double atmRad = 1 + atmHeight;
        double atmRad2 = sqr(atmRad);

        spec.buffer.free();
        spec.width = 512;
        spec.height = 512;
        spec.components = 4;

        std::string name;
        {
            std::stringstream ss;
            ss << std::setprecision(7) << std::fixed
               << "density-" << spec.width << "-" << spec.height << "-"
               << atmRad << "-" << body.atmosphere.verticalExponent << ".png";
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
            spec.buffer.allocate(spec.width * spec.height * 4);
            unsigned char *valsArray = (unsigned char*)spec.buffer.data();

            // actually generate the texture content
            for (uint32 xx = 0; xx < spec.width; xx++)
            {
                std::stringstream ss;
                ss << "Atmosphere progress: " << xx << " / " << spec.width;
                vts::log(vts::LogLevel::info1, ss.str());

                double cosfi = 2 * xx / (double)spec.width - 1;
                double fi = std::acos(cosfi);
                double sinfi = std::sin(fi);

                for (uint32 yy = 0; yy < spec.height; yy++)
                {
                    double yyy = yy / (double)spec.height;
                    double r = 2 * atmHeight * yyy - atmHeight + 1;
                    double t0 = cosfi * r;
                    double y = sinfi * r;
                    double y2 = sqr(y);
                    double a = sqrt(atmRad2 - y2);
                    double density = 0;
                    static const double step = 0.0003;
                    for (double t = t0; t < a; t += step)
                    {
                        double h = std::sqrt(sqr(t) + y2);
                        h = (clamp(h, 1, atmRad) - 1) / atmHeight;
                        double a = std::exp(h
                            * -body.atmosphere.verticalExponent);
                        density += a;
                    }
                    density *= step;
                    encodeFloat(density * 0.2,
                                valsArray + ((yy * spec.width + xx) * 4));
                }
            }

            // save the texture to file
            {
                vts::log(vts::LogLevel::info3,
                         std::string() + "The atmosphere texture "
                         "will be saved to file <" + name + ">");
                try
                {
                    Buffer b = spec.encodePng();
                    writeLocalFileBuffer(name, b);
                }
                catch (...)
                {
                    // dont care
                }
            }
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

