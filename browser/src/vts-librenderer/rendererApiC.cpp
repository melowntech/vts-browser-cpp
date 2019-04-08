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
    vts::renderer::checkGlImpl(name);
    C_END
}

void vtsCheckGlFramebuffer(uint32 target)
{
    C_BEGIN
    vts::renderer::checkGlFramebuffer(target);
    C_END
}

void vtsLoadGlFunctions(GLADloadproc functionLoader)
{
    C_BEGIN
    vts::renderer::loadGlFunctions(functionLoader);
    C_END
}

typedef struct vtsCRenderContext
{
    std::shared_ptr<vts::renderer::RenderContext> p;
} vtsCRenderContext;

typedef struct vtsCRenderView
{
    std::shared_ptr<vts::renderer::RenderView> p;
} vtsCRenderView;

vtsHRenderContext vtsRenderContextCreate()
{
    C_BEGIN
    vtsHRenderContext r = new vtsCRenderContext();
    r->p = std::make_shared<vts::renderer::RenderContext>();
    return r;
    C_END
    return nullptr;
}

void vtsRenderContextDestroy(vtsHRenderContext context)
{
    C_BEGIN
    delete context;
    C_END
}

void vtsRenderContextBindLoadFunctions(vtsHRenderContext context,
    vtsHMap map)
{
    C_BEGIN
    context->p->bindLoadFunctions(map->p.get());
    C_END
}

vtsHRenderView vtsRenderContextCreateView(vtsHRenderContext context,
    vtsHCamera camera)
{
    C_BEGIN
    vtsHRenderView r = new vtsCRenderView();
    r->p = context->p->createView(camera->p.get());
    return r;
    C_END
    return nullptr;
}

void vtsRenderViewDestroy(vtsHRenderView view)
{
    C_BEGIN
    delete view;
    C_END
}

vtsCRenderOptionsBase *vtsRenderViewOptions(
    vtsHRenderView view)
{
    C_BEGIN
    return &view->p->options();
    C_END
    return nullptr;
}

const vtsCRenderVariablesBase *vtsRenderViewVariables(
    vtsHRenderView view)
{
    C_BEGIN
    return &view->p->variables();
    C_END
    return nullptr;
}

void vtsRenderViewRender(vtsHRenderView view)
{
    C_BEGIN
    view->p->render();
    C_END
}

void vtsRenderViewRenderCompas(vtsHRenderView view,
    const double screenPosSize[3], const double mapRotation[3])
{
    C_BEGIN
    view->p->renderCompass(screenPosSize, mapRotation);
    C_END
}

void vtsRenderViewGetWorldPosition(vtsHRenderView view,
    const double screenPosIn[2], double worldPosOut[3])
{
    C_BEGIN
    view->p->getWorldPosition(screenPosIn, worldPosOut);
    C_END
}

#ifdef __cplusplus
} // extern C
#endif
