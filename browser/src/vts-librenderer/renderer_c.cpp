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

#include "include/vts-renderer/renderer.h"
#include "../vts-libbrowser/mapApiC.hpp"

#include "include/vts-renderer/renderer.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void vtsCheckGl(const char *name)
{
    C_BEGIN
    vts::renderer::checkGl(name);
    C_END
}

void vtsCheckGlFramebuffer()
{
    C_BEGIN
    vts::renderer::checkGlFramebuffer();
    C_END
}

void vtsLoadGlFunctions(GLADloadproc functionLoader)
{
    C_BEGIN
    vts::renderer::loadGlFunctions(functionLoader);
    C_END
}

typedef struct vtsCRenderer
{
    std::shared_ptr<vts::renderer::Renderer> p;
} vtsCRenderer;

vtsHRenderer vtsRendererCreate()
{
    C_BEGIN
    vtsHRenderer r = new vtsCRenderer();
    r->p = std::make_shared<vts::renderer::Renderer>();
    return r;
    C_END
    return nullptr;
}

void vtsRendererDestroy(vtsHRenderer renderer)
{
    C_BEGIN
    delete renderer;
    C_END
}

void vtsRendererInitialize(vtsHRenderer renderer)
{
    C_BEGIN
    renderer->p->initialize();
    C_END
}

void vtsRendererFinalize(vtsHRenderer renderer)
{
    C_BEGIN
    renderer->p->finalize();
    C_END
}

void vtsRendererBindLoadFunctions(vtsHRenderer renderer,
                                   vtsHMap map)
{
    C_BEGIN
    renderer->p->bindLoadFunctions(map->p.get());
    C_END
}

vtsCRenderOptionsBase *vtsRendererOptions(
                                   vtsHRenderer renderer)
{
    C_BEGIN
    return &renderer->p->options();
    C_END
    return nullptr;
}

const vtsCRenderVariablesBase *vtsRendererVariables(
                                   vtsHRenderer renderer)
{
    C_BEGIN
    return &renderer->p->variables();
    C_END
    return nullptr;
}

void vtsRendererRender(vtsHRenderer renderer, vtsHMap map)
{
    C_BEGIN
    renderer->p->render(map->p.get());
    C_END
}

void vtsRendererRenderCompas(vtsHRenderer renderer,
                       const double screenPosSize[3],
                       const double mapRotation[3])
{
    C_BEGIN
    renderer->p->renderCompass(screenPosSize, mapRotation);
    C_END
}

void vtsRendererGetWorldPosition(vtsHRenderer renderer,
                       const double screenPosIn[2],
                       double worldPosOut[3])
{
    C_BEGIN
    renderer->p->getWorldPosition(screenPosIn, worldPosOut);
    C_END
}

#ifdef __cplusplus
} // extern C
#endif
