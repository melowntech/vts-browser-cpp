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

#ifndef RENDERER_H_sghvfwjfn
#define RENDERER_H_sghvfwjfn

#include <vts-browser/resources.hpp>
#include <vts-browser/draws.hpp>
#include <vts-browser/celestial.hpp>

#include "foundation.hpp"

#ifndef VTSR_INCLUDE_GL
typedef void* (* GLADloadproc)(const char *name);
#endif

namespace vts { namespace renderer
{

// initialize all gl functions
// should be called once after the gl context has been created
VTSR_API void loadGlFunctions(GLADloadproc functionLoader);

// can be directly bound to MapCallbacks
VTSR_API void loadTexture(vts::ResourceInfo &info,
                          const vts::GpuTextureSpec &spec);

// can be directly bound to MapCallbacks
VTSR_API void loadMesh(vts::ResourceInfo &info,
                       const vts::GpuMeshSpec &spec);

// load all shaders and initialize all state required for the rendering
// should be called once after the gl functions were initialized
VTSR_API void initialize();

// clear the loaded shaders etc.
// should be called once before the gl context is released
VTSR_API void finalize();

struct VTSR_API RenderOptions
{
    int width;
    int height;
    int targetFrameBuffer;
    int targetViewportX;
    int targetViewportY;
    int antialiasingSamples;
    bool renderAtmosphere;
    bool renderPolygonEdges;

    RenderOptions();
};

VTSR_API void render(RenderOptions &options,
                     const MapDraws &draws,
                     const MapCelestialBody &celestialBody);

// reconstruct world position for mouse picking
// uses data from last call to render
VTSR_API void getWorldPosition(const double screenPosIn[2], double worldPosOut[3]);

} } // namespace vts::renderer

#endif
