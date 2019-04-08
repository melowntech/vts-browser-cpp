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

#include <vts-browser/map.hpp>
#include <vts-browser/mapCallbacks.hpp>
#include <vts-browser/camera.hpp>

#include "renderer.hpp"

namespace vts { namespace renderer
{

RenderOptions::RenderOptions()
{
    memset(this, 0, sizeof(*this));
    textScale = 1.5;
    antialiasingSamples = 1;
    renderAtmosphere = true;
    colorToTargetFrameBuffer = true;
}

RenderVariables::RenderVariables()
{
    memset(this, 0, sizeof(*this));
}

RenderContext::RenderContext()
{
    impl = std::make_shared<RenderContextImpl>(this);
}

RenderContext::~RenderContext()
{}

void RenderContext::bindLoadFunctions(Map *map)
{
    assert(map);
    map->callbacks().loadTexture = std::bind(&RenderContext::loadTexture, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    map->callbacks().loadMesh = std::bind(&RenderContext::loadMesh, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    map->callbacks().loadFont = std::bind(&RenderContext::loadFont, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    map->callbacks().loadGeodata = std::bind(&RenderContext::loadGeodata, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

std::shared_ptr<RenderView> RenderContext::createView(Camera *cam)
{
    assert(cam);
    return std::make_shared<RenderView>(impl.get(), cam);
}

RenderView::RenderView(RenderContextImpl *context, Camera *cam)
{
    assert(context);
    assert(cam);
    impl = std::make_shared<RenderViewImpl>(cam, this, context);
}

Camera *RenderView::camera()
{
    return impl->camera;
}

RenderOptions &RenderView::options()
{
    return impl->options;
}

const RenderVariables &RenderView::variables() const
{
    return impl->vars;
}

void RenderView::render()
{
    impl->draws = &impl->camera->draws();
    impl->body = &impl->camera->map()->celestialBody();
    impl->projected = impl->camera->map()->getMapProjected();
    impl->atmosphereDensityTexture
        = (Texture*)impl->camera->map()->atmosphereDensityTexture().get();
    impl->elapsedTime = impl->camera->map()->lastRenderUpdateElapsedTime();
    impl->render();
    impl->draws = nullptr;
    impl->body = nullptr;
    impl->atmosphereDensityTexture = nullptr;
}

void RenderView::renderCompass(const double screenPosSize[3],
                   const double mapRotation[3])
{
    impl->renderCompass(screenPosSize, mapRotation);
}

void RenderView::getWorldPosition(const double screenPosIn[2],
                      double worldPosOut[3])
{
    impl->getWorldPosition(screenPosIn, worldPosOut);
}

} } // namespace vts renderer

