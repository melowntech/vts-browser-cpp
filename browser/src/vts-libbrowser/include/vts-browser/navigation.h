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

#ifndef NAVIGATION_H_sgrhgf
#define NAVIGATION_H_sgrhgf

#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

// creation and destruction
VTS_API vtsHNavigation vtsNavigationCreate(vtsHCamera cam);
VTS_API void vtsNavigationDestroy(vtsHNavigation map);

// navigation
VTS_API void vtsNavigationPan(vtsHNavigation nav, const double value[3]);
VTS_API void vtsNavigationRotate(vtsHNavigation nav, const double value[3]);
VTS_API void vtsNavigationZoom(vtsHNavigation nav, double value);
VTS_API void vtsNavigationResetAltitude(vtsHNavigation nav);
VTS_API void vtsNavigationResetNavigationMode(vtsHNavigation nav);

// setters
VTS_API void vtsNavigationSetSubjective(vtsHNavigation nav,
                    bool subjective, bool convert);
VTS_API void vtsNavigationSetPoint(vtsHNavigation nav,
                    const double point[3]);
VTS_API void vtsNavigationSetRotation(vtsHNavigation nav,
                    const double point[3]);
VTS_API void vtsNavigationSetViewExtent(vtsHNavigation nav,
                    double viewExtent);
VTS_API void vtsNavigationSetFov(vtsHNavigation nav, double fov);
VTS_API void vtsNavigationSetAutoRotation(vtsHNavigation nav, double value);
VTS_API void vtsNavigationSetPositionJson(vtsHNavigation nav,
                    const char *position);
VTS_API void vtsNavigationSetPositionUrl(vtsHNavigation nav,
                    const char *position);

// getters
VTS_API bool vtsNavigationGetSubjective(vtsHNavigation nav);
VTS_API void vtsNavigationGetPoint(vtsHNavigation nav, double point[3]);
VTS_API void vtsNavigationGetRotation(vtsHNavigation nav, double rot[3]);
VTS_API void vtsNavigationGetRotationLimited(vtsHNavigation nav,
                    double rot[3]);
VTS_API double vtsNavigationGetViewExtent(vtsHNavigation nav);
VTS_API double vtsNavigationGetFov(vtsHNavigation nav);
VTS_API double vtsNavigationGetAutoRotation(vtsHNavigation nav);
VTS_API const char *vtsNavigationGetPositionUrl(vtsHNavigation nav);
VTS_API const char *vtsNavigationGetPositionJson(vtsHNavigation nav);

// options
VTS_API const char *vtsNavigationGetOptions(vtsHNavigation nav);
VTS_API void vtsNavigationSetOptions(vtsHNavigation nav, const char *options);

#ifdef __cplusplus
} // extern C
#endif

#endif
