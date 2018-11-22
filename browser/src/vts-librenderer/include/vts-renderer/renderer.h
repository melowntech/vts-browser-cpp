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

#ifndef RENDERER_H_crfweifh
#define RENDERER_H_crfweifh

#include "renderer_common.h"
#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtsCRenderer *vtsHRenderer;

VTSR_API vtsHRenderer vtsRendererCreate();
VTSR_API void vtsRendererDestroy(vtsHRenderer renderer);
VTSR_API void vtsRendererInitialize(vtsHRenderer renderer);
VTSR_API void vtsRendererFinalize(vtsHRenderer renderer);
VTSR_API void vtsRendererBindLoadFunctions(vtsHRenderer renderer,
                                            vtsHMap map);
VTSR_API vtsCRenderOptionsBase *vtsRendererOptions(
                                            vtsHRenderer renderer);
VTSR_API const vtsCRenderVariablesBase *vtsRendererVariables(
                                            vtsHRenderer renderer);
VTSR_API void vtsRendererRender(vtsHRenderer renderer, vtsHCamera camera);
VTSR_API void vtsRendererRenderCompas(vtsHRenderer renderer,
                                const double screenPosSize[3],
                                const double mapRotation[3]);
VTSR_API void vtsRendererGetWorldPosition(vtsHRenderer renderer,
                                const double screenPosIn[2],
                                double worldPosOut[3]);

#ifdef __cplusplus
} // extern C
#endif

#endif
