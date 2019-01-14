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

#ifndef MAP_H_sgrhgf
#define MAP_H_sgrhgf

#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

// map creation and destruction
VTS_API vtsHMap vtsMapCreate(const char *createOptions, vtsHFetcher fetcher);
VTS_API void vtsMapDestroy(vtsHMap map);

// custom data
VTS_API void vtsMapSetCustomData(vtsHMap map, void *data);
VTS_API void *vtsMapGetCustomData(vtsHMap map);

// config path
VTS_API void vtsMapSetConfigPaths(vtsHMap map, const char *mapconfigPath,
                                  const char *authPath);
VTS_API const char *vtsMapGetConfigPath(vtsHMap map);

// readyness status
VTS_API bool vtsMapGetConfigAvailable(vtsHMap map);
VTS_API bool vtsMapGetConfigReady(vtsHMap map);
VTS_API bool vtsMapGetProjected(vtsHMap map);
VTS_API bool vtsMapGetRenderComplete(vtsHMap map);
VTS_API double vtsMapGetRenderProgress(vtsHMap map);

// data processing (may be run on a dedicated thread)
VTS_API void vtsMapDataInitialize(vtsHMap map);
VTS_API void vtsMapDataUpdate(vtsHMap map);
VTS_API void vtsMapDataFinalize(vtsHMap map);
VTS_API void vtsMapDataAllRun(vtsHMap map);

// rendering
VTS_API void vtsMapRenderInitialize(vtsHMap map);
VTS_API void vtsMapRenderUpdate(vtsHMap map, double elapsedTime); // seconds since last call
VTS_API void vtsMapRenderFinalize(vtsHMap map);

// options and statistics
VTS_API const char *vtsMapGetOptions(vtsHMap map);
VTS_API const char *vtsMapGetStatistics(vtsHMap map);
VTS_API void vtsMapSetOptions(vtsHMap map, const char *options);

// conversion
VTS_API void vtsMapConvert(vtsHMap map,
                           const double pointFrom[3], double pointTo[3],
                           uint32 srsFrom, uint32 SrsTo);

// map view functionality is not yet available in the C API

#ifdef __cplusplus
} // extern C
#endif

#endif
