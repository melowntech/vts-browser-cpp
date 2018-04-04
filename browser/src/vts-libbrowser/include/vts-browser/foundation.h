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

#ifndef FOUNDATION_H_dehgjdgn
#define FOUNDATION_H_dehgjdgn

#include "foundation_common.h"
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t uint8;
typedef int8_t sint8;
typedef uint16_t uint16;
typedef int16_t sint16;
typedef uint32_t uint32;
typedef int32_t sint32;
typedef uint64_t uint64;
typedef int64_t sint64;

#ifdef __cplusplus
extern "C" {
#endif

// every call to the C API functions may set an error
// the error is stored in a thread-local storage accessible through
//   vtsErrCode() and vtsErrMsg()

// 0 = no error
VTS_API sint32 vtsErrCode();

// some error states may have associated an additional description of the error
VTS_API const char *vtsErrMsg();

// helper for translating an error code into human readable message
VTS_API const char *vtsErrCodeToName(sint32 code);

// reset internal error state to ok
VTS_API void vtsErrClear();

// opaque structures and handles
typedef struct vtsCMap *vtsHMap;
typedef struct vtsCFetcher *vtsHFetcher;
typedef struct vtsCResource *vtsHResource;

#ifdef __cplusplus
} // extern C
#endif

#endif

