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

#ifndef RENDERER_COMMON_H_uisgzfw
#define RENDERER_COMMON_H_uisgzfw

#include "foundationCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

// options provided from the application (you set these)
typedef struct vtsCContextOptionsBase
{
    // when using separate thread with separate OpenGL context for uploading
    //   data to GPU memory it is required to call glFinish to wait for
    //   the data to be completely uploaded before they can be used in
    //   the rendering thread
    // calling glFinish when data uploads and rendering are done on the same
    //   thread/context can lead to pointless and significant performance
    //   degradation and can therefore be changed here
    bool callGlFinishAfterUploadingData;
} vtsCContextOptionsBase;

// options provided from the application (you set these)
typedef struct vtsCRenderOptionsBase
{
    // additional scaling of geodata text rendering
    float textScale;

    // render resolution in pixels
    uint32 width;
    uint32 height;

    // when colorToTargetFrameBuffer is true, the render method will blit
    //   the resulting color into this frame buffer
    // it will overwrite any previous data
    uint32 targetFrameBuffer;
    uint32 targetViewportX;
    uint32 targetViewportY;
    uint32 targetViewportW; // zero will use the render width
    uint32 targetViewportH; // zero will use the render height

    // other options
    uint32 antialiasingSamples; // two or more to enable multisampling
    uint32 debugGeodataMode; // 0 = disabled
    bool renderAtmosphere;
    bool geodataHysteresis;
    bool colorRenderWithAlpha;
    bool debugFlatShading;
    bool debugWireframe;
    bool debugDepthFeedback;

    // where to copy the result (and resolve multisampling)
    bool colorToTargetFrameBuffer;
    bool colorToTexture; // accessible as RenderVariables::colorReadTexId
} vtsCRenderOptionsBase;

// these variables are controlled by the library
//   and are provided to you for potential further use
// do not change any attributes on the objects!
typedef struct vtsCRenderVariablesBase
{
    uint32 frameRenderBufferId;
    uint32 frameReadBufferId; // (may be same as frameRenderBufferId)
    uint32 depthRenderTexId; // has type textureTargetType
    uint32 depthReadTexId; // GL_TEXTURE_2D (may be same as depthRenderTexId)
    uint32 colorRenderTexId; // has type textureTargetType
    uint32 colorReadTexId; // GL_TEXTURE_2D (may be same as colorRenderTexId)
    uint32 textureTargetType; // GL_TEXTURE_2D or GL_TEXTURE_2D_MULTISAMPLE
} vtsCRenderVariablesBase;

#ifdef __cplusplus
} // extern C
#endif

#endif
