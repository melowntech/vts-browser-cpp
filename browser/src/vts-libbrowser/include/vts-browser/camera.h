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

#ifndef CAMERA_H_sdjigsauzf
#define CAMERA_H_sdjigsauzf

#include "camera_common.h"

#ifdef __cplusplus
extern "C" {
#endif

VTS_API vtsHCamera vtsCameraCreate(vtsHMap map);
VTS_API void vtsCameraDestroy(vtsHCamera cam);

// camera
VTS_API void vtsCameraSetViewportSize(vtsHCamera cam,
                    uint32 width, uint32 height);
VTS_API void vtsCameraSetView(vtsHCamera cam, const double eye[3],
                    const double target[3], const double up[3]);
VTS_API void vtsCameraSetViewMatrix(vtsHCamera cam, const double view[16]);
VTS_API void vtsCameraSetProj(vtsHCamera cam, double fovyDegs,
                    double near_, double far_);
VTS_API void vtsCameraSetProjMatrix(vtsHCamera cam, const double proj[16]);
VTS_API void vtsCameraGetViewportSize(vtsHCamera cam,
                    uint32 *width, uint32 *height);
VTS_API void vtsCameraGetView(vtsHCamera cam, double eye[3],
                    double target[3], double up[3]);
VTS_API void vtsCameraGetViewMatrix(vtsHCamera cam, double view[16]);
VTS_API void vtsCameraGetProjMatrix(vtsHCamera cam, double proj[16]);
VTS_API void vtsCameraSuggestedNearFar(vtsHCamera cam,
                    double *near_, double *far_);
VTS_API void vtsCameraRenderUpdate(vtsHCamera cam);

// credits
VTS_API const char *vtsCameraGetCredits(vtsHCamera cam);
VTS_API const char *vtsCameraGetCreditsShort(vtsHCamera cam);
VTS_API const char *vtsCameraGetCreditsFull(vtsHCamera cam);

// options & statistics
VTS_API const char *vtsCameraGetOptions(vtsHCamera cam);
VTS_API const char *vtsCameraGetStatistics(vtsHCamera cam);
VTS_API void vtsCameraSetOptions(vtsHCamera cam, const char *options);

// acquire iterator for the draw tasks
VTS_API vtsHDrawsGroup vtsDrawsOpaque(vtsHCamera cam);
VTS_API vtsHDrawsGroup vtsDrawsTransparent(vtsHCamera cam);
VTS_API vtsHDrawsGroup vtsDrawsGeodata(vtsHCamera cam);
VTS_API vtsHDrawsGroup vtsDrawsInfographics(vtsHCamera cam);
VTS_API vtsHDrawsGroup vtsDrawsColliders(vtsHCamera cam);
VTS_API uint32 vtsDrawsCount(vtsHDrawsGroup group);
VTS_API void vtsDrawsDestroy(vtsHDrawsGroup group);

// accesors for individual data pointed to by the iterator
VTS_API void *vtsDrawsMesh(vtsHDrawsGroup group, uint32 index);
VTS_API void *vtsDrawsTexColor(vtsHDrawsGroup group, uint32 index);
VTS_API void *vtsDrawsTexMask(vtsHDrawsGroup group, uint32 index);
VTS_API const vtsCDrawBase *vtsDrawsDetail(vtsHDrawsGroup group, uint32 index);

// accesor for all data pointed to by the iterator
// (this function is subject to more frequent changes)
VTS_API const vtsCDrawBase *vtsDrawsAllInOne(vtsHDrawsGroup group,
                    uint32 index, void **mesh,
                    void **texColor, void **texMask);

VTS_API const vtsCCameraBase *vtsDrawsCamera(vtsHCamera cam);

VTS_API void *vtsDrawsAtmosphereDensityTexture(vtsHMap map);

#ifdef __cplusplus
} // extern C
#endif

#endif

