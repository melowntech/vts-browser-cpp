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

#ifndef DRAWS_H_sdjigsauzf
#define DRAWS_H_sdjigsauzf

#include "draws_common.h"
#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

VTS_API const vtsCCameraBase *vtsDrawsCamera(vtsHMap map);

typedef struct vtsCDrawIterator *vtsHDrawIterator;

// acquire iterator for the draw tasks
VTS_API vtsHDrawIterator vtsDrawsOpaque(vtsHMap map);
VTS_API vtsHDrawIterator vtsDrawsTransparent(vtsHMap map);
VTS_API vtsHDrawIterator vtsDrawsGeodata(vtsHMap map);
VTS_API vtsHDrawIterator vtsDrawsInfographic(vtsHMap map);
VTS_API bool vtsDrawsNext(vtsHDrawIterator iterator);
VTS_API void vtsDrawsDestroy(vtsHDrawIterator iterator);

// accesors for individual data pointed to by the iterator
VTS_API void *vtsDrawsMesh(vtsHDrawIterator iterator);
VTS_API void *vtsDrawsTexColor(vtsHDrawIterator iterator);
VTS_API void *vtsDrawsTexMask(vtsHDrawIterator iterator);
VTS_API const vtsCDrawBase *vtsDrawsDetail(vtsHDrawIterator iterator);

// accesor for all data pointed to by the iterator
// (this function is subject to more frequent changes)
VTS_API bool vtsDrawsAllInOne(vtsHDrawIterator iterator,
                              const vtsCDrawBase **details,
                              void **mesh, void **texColor, void **texMask);

#ifdef __cplusplus
} // extern C
#endif

/* EXAMPLE:

vtsHDrawIterator it = vtsDrawsOpaque(map);
while (it)
{
    // access data asociated with the draw task
    void *mesh = vtsDrawsMesh(it);
    void *texColor = vtsDrawsTexColor(it);
    void *texMask = vtsDrawsTexMask(it);
    const vtsCDrawBase *details = vtsDrawsDetail(it);

    // dispatch the draw task
    ...

    // fetch next draw task
    if (!vtsDrawsNext(it))
    {
        vtsDrawsDestroy(it);
        break;
    }
}

*/

/* ALTERNATIVE EXAMPLE:

vtsHDrawIterator it = vtsDrawsOpaque(map);
while (it)
{
    // access data asociated with the draw task
    void *mesh;
    void *texColor;
    void *texMask;
    const vtsCDrawBase *details;
    bool next = vtsDrawsAllInOne(it, details, mesh, texColor, texMask);

    // dispatch the draw task
    ...

    // fetch next draw task
    if (!next)
    {
        vtsDrawsDestroy(it);
        break;
    }
}

*/

#endif

