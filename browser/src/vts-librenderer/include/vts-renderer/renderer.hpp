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

#ifndef RENDERER_HPP_sghvfwjfn
#define RENDERER_HPP_sghvfwjfn

#include <string>
#include <memory>

#include "rendererCommon.h"
#include "foundation.hpp"

namespace vts
{

class Map;
class Camera;
class MapCelestialBody;
class ResourceInfo;
class GpuTextureSpec;
class GpuMeshSpec;
class GpuFontSpec;
class GpuGeodataSpec;

namespace renderer
{

class RenderContext;
class RenderViewImpl;
class RenderContextImpl;

struct VTSR_API ContextOptions : public vtsCContextOptionsBase
{
    ContextOptions();
};

struct VTSR_API RenderOptions : public vtsCRenderOptionsBase
{
    RenderOptions();
};

struct VTSR_API RenderVariables : public vtsCRenderVariablesBase
{
    RenderVariables();
};

class VTSR_API RenderView
{
public:
    RenderView(RenderContextImpl *context, Camera *cam);

    Camera *camera();
    RenderOptions &options();
    const RenderVariables &variables() const;
    void render();

    // reconstruct world position for mouse picking
    // uses data from last call to render
    // returns NaN if the position cannot be obtained
    void getWorldPosition(const double screenPosIn[2],
        double worldPosOut[3]);

    void renderCompass(const double screenPosSize[3],
        const double mapRotation[3]);

private:
    std::shared_ptr<RenderViewImpl> impl;
    friend RenderContext;
};

class VTSR_API RenderContext
{
public:
    // load all shaders and initialize all state required for the rendering
    RenderContext();

    // clear the loaded shaders etc.
    ~RenderContext();

    ContextOptions &options();

    // can be directly bound to MapCallbacks
    void loadTexture(ResourceInfo &info, GpuTextureSpec &spec,
        const std::string &debugId);
    void loadMesh(ResourceInfo &info, GpuMeshSpec &spec,
        const std::string &debugId);
    void loadFont(ResourceInfo &info, GpuFontSpec &spec,
        const std::string &debugId);
    void loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec,
        const std::string &debugId);
    void bindLoadFunctions(Map *map);

    // create new render view
    // you may have multiple views in single render context
    std::shared_ptr<RenderView> createView(Camera *cam);

private:
    std::shared_ptr<RenderContextImpl> impl;
};

} // namespace renderer
} // namespace vts

#endif
