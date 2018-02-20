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

#include <assert.h>
#include <mutex>
#include <thread>
#include <sstream>
#include <iomanip>

#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>

#include "atmosphereDensityTexture.hpp"

namespace vts { namespace renderer
{

using namespace priv;

namespace
{

std::mutex mut;

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

void atmosphereThreadEntry(MapCelestialBody body, int thrIdx)
{
    try
    {
        vts::setLogThreadName("atmosphere");
        vts::log(vts::LogLevel::info2, "Loading atmosphere density texture");

        double atmHeight = body.atmosphere.thickness / body.majorRadius;
        double atmRad = 1 + atmHeight;
        double atmRad2 = sqr(atmRad);

        GpuTextureSpec spec;
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

        // pass the texture to the main thread
        {
            std::lock_guard<std::mutex> lck(mut);
            if (atmosphere.thrIdx == thrIdx)
            {
                atmosphere.spec = std::move(spec);
                atmosphere.state = 2;
            }
        }

        vts::log(vts::LogLevel::info2, "Finished atmosphere density texture");
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

} // namespace

namespace priv
{

AtmosphereProp::AtmosphereProp() : state(0), thrIdx(0)
{}

bool AtmosphereProp::validate(const vts::MapCelestialBody &current)
{
    if (current.majorRadius == 0 || current.atmosphere.thickness == 0)
    {
        state = 0;
        return false;
    }
    if (!areSame(current, body) || state == 0)
    {
        std::lock_guard<std::mutex> lck(mut);
        thrIdx++;
        state = 1;
        body = current;
        std::thread thr(&atmosphereThreadEntry, body, (int)thrIdx);
        thr.detach();
    }
    if (state == 2)
    {
        std::lock_guard<std::mutex> lck(mut);
        tex = std::make_shared<Texture>();
        vts::ResourceInfo info;
        tex->load(info, spec);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        spec.buffer.free();
        state = 3;
    }
    return state == 3;
}

AtmosphereProp atmosphere;

} // namespace priv


} } // namespace vts renderer

