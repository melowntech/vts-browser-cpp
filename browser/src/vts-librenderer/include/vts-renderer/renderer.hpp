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

#include "foundation.hpp"

namespace vts
{

class Map;
class MapDraws;
class MapCelestialBody;
class ResourceInfo;
class GpuTextureSpec;
class GpuMeshSpec;

namespace renderer
{

// options provided from the application (you set these)
struct VTSR_API RenderOptions
{
    // render resolution in pixels
    uint32 width;
    uint32 height;

    // when colorToTargetFrameBuffer is true, the render method will blit
    //   the resulting color into this frame buffer
    // it will overwrite any previous data
    uint32 targetFrameBuffer;
    uint32 targetViewportX;
    uint32 targetViewportY;

    // other options
    uint32 antialiasingSamples; // two or more to enable multisampling
    bool renderAtmosphere;
    bool renderPolygonEdges;

    // where to copy the result (and resolve multisampling)
    bool colorToTargetFrameBuffer;
    bool colorToTexture; // accessible as RenderVariables::colorReadTexId

    RenderOptions();
};

// these variables are controlled by the library
//   and are provided to you for potential further use
// do not change any attributes on the objects!
struct VTSR_API RenderVariables
{
    uint32 frameRenderBufferId;
    uint32 frameReadBufferId; // (may be same as frameRenderBufferId)
    uint32 depthRenderTexId; // has type textureTargetType
    uint32 depthReadTexId; // GL_TEXTURE_2D (may be same as depthRenderTexId)
    uint32 colorRenderTexId; // has type textureTargetType
    uint32 colorReadTexId; // GL_TEXTURE_2D (may be same as colorRenderTexId)
    uint32 textureTargetType; // GL_TEXTURE_2D or GL_TEXTURE_2D_MULTISAMPLE
    RenderVariables();
};

class VTSR_API Renderer
{
public:
    Renderer();
    ~Renderer();

    // load all shaders and initialize all state required for the rendering
    // should be called once after the gl functions were initialized
    void initialize();

    // clear the loaded shaders etc.
    // should be called once before the gl context is released
    void finalize();

    // can be directly bound to MapCallbacks
    void loadTexture(ResourceInfo &info, GpuTextureSpec &spec);
    void loadMesh(ResourceInfo &info, GpuMeshSpec &spec);
    void bindLoadFunctions(Map *map);

    RenderOptions &options();
    const RenderVariables &variables() const;

    void render(const MapDraws &draws,
                const MapCelestialBody &celestialBody);
    void render(Map *map);

    void renderCompass(const double screenPosSize[3],
                       const double mapRotation[3]);

    // reconstruct world position for mouse picking
    // uses data from last call to render (requires access to opengl context)
    // returns NaN if the position cannot be obtained
    void getWorldPosition(const double screenPosIn[2],
                          double worldPosOut[3]);

private:
    std::shared_ptr<class RendererImpl> impl;
};

} // namespace renderer
} // namespace vts

#endif
