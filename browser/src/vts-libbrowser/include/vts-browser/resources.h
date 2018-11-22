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

#ifndef RESOURCES_H_vgfswbh
#define RESOURCES_H_vgfswbh

#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

VTS_API uint32 vtsGpuTypeSize(uint32 type);

// the vtsHResource handle is only valid inside the load* callbacks,
//   you may not use it from outside the corresponding call

typedef void (*vtsResourceDeleterCallbackType)(void *ptr);

// resource
VTS_API void vtsResourceSetUserData(vtsHResource resource, void *data,
                                    vtsResourceDeleterCallbackType deleter);
VTS_API void vtsResourceSetMemoryCost(vtsHResource resource,
                                      uint32 ramMem, uint32 gpuMem);

// texture
VTS_API void vtsTextureGetResolution(vtsHResource resource,
                uint32 *width, uint32 *height, uint32 *components);
VTS_API uint32 vtsTextureGetType(vtsHResource resource);
VTS_API uint32 vtsTextureGetInternalFormat(vtsHResource resource);
VTS_API uint32 vtsTextureGetFilterMode(vtsHResource resource);
VTS_API uint32 vtsTextureGetWrapMode(vtsHResource resource);
VTS_API void vtsTextureGetBuffer(vtsHResource resource,
                void **data, uint32 *size);

// mesh
VTS_API uint32 vtsMeshGetFaceMode(vtsHResource resource);
VTS_API void vtsMeshGetVertices(vtsHResource resource,
                void **data, uint32 *size, uint32 *count); // size is total size of the buffer in bytes
VTS_API void vtsMeshGetIndices(vtsHResource resource,
                void **data, uint32 *size, uint32 *count);
VTS_API void vtsMeshGetAttribute(vtsHResource resource, uint32 index,
                uint32 *offset, uint32 *stride, uint32 *components,
                uint32 *type, bool *enable, bool *normalized);

#ifdef __cplusplus
} // extern C
#endif

#endif
