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

#ifndef FOUNDATION_H_qwergfbhjk
#define FOUNDATION_H_qwergfbhjk

#include <assert.h>
#include <sstream>
#include <iomanip>

#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>
#include <vts-browser/buffer.hpp>
#include <vts-browser/resources.hpp>
#include <vts-browser/draws.hpp>
#include <vts-browser/celestial.hpp>

#include "include/vts-renderer/classes.hpp"
#include "include/vts-renderer/renderer.hpp"

namespace vts { namespace renderer
{

namespace priv
{

extern uint32 maxAntialiasingSamples;
extern float maxAnisotropySamples;

struct AtmosphereDensity
{
    AtmosphereDensity();
    ~AtmosphereDensity();

    void initialize();
    void finalize();

    Texture *validate(const MapCelestialBody &current);

private:
    std::shared_ptr<class AtmosphereImpl> impl;
};

} // namespace priv

using namespace priv;

} } // namespace vts renderer

#endif
