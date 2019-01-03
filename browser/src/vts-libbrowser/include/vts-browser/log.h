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

#ifndef LOG_H_sdghfa45
#define LOG_H_sdghfa45

#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vtsLogCallbackType)(const char *msg);

VTS_API void vtsLogSetMaskStr(const char *mask);
VTS_API void vtsLogSetMaskCode(uint32 mask);
VTS_API void vtsLogSetConsole(bool enable);
VTS_API void vtsLogSetFile(const char *filename);
VTS_API void vtsLogSetThreadName(const char *name);
VTS_API void vtsLogAddSink(uint32 mask, vtsLogCallbackType callback);
VTS_API void vtsLogClearSinks();

// this function will never set an error
// if anything happens during the logging, it is silently ignored
VTS_API void vtsLog(uint32 level, const char *message);

enum
{
    vtsLogLevelDebug =    0x0000fu,
    vtsLogLevelInfo1 =    0x000f0u,
    vtsLogLevelInfo2 =    0x00070u,
    vtsLogLevelInfo3 =    0x00030u,
    vtsLogLevelInfo4 =    0x00010u,
    vtsLogLevelWarn1 =    0x00f00u,
    vtsLogLevelWarn2 =    0x00700u,
    vtsLogLevelWarn3 =    0x00300u,
    vtsLogLevelWarn4 =    0x00100u,
    vtsLogLevelErr1 =     0x0f000u,
    vtsLogLevelErr2 =     0x07000u,
    vtsLogLevelErr3 =     0x03000u,
    vtsLogLevelErr4 =     0x01000u,
    vtsLogLevelFatal =    0xf0000u,
};

#ifdef __cplusplus
} // extern C
#endif

#endif
