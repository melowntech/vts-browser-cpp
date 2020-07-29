/**
 * Copyright (c) 2020 Melown Technologies SE
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

#ifndef guard_highPerformanceGpuHint_h_56esr4hgrt6
#define guard_highPerformanceGpuHint_h_56esr4hgrt6

#include <vts-browser/foundationCommon.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
Defining these symbols gives a hint to the system that, when creating OpenGL context,
it should choose high performance gpu, if multiple gpus were available.
This is specifically useful for laptops with switchable gpus.

These symbols have to be defined in the actual application, not in a library,
therefore we provide it in the form of an include header.
Make sure that the header is included in exactly one translation unit (one c or cpp file), preferably the main file.
*/

VTS_API_EXPORT int NvOptimusEnablement = 1;
VTS_API_EXPORT int AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
} // extern C
#endif

#endif // guard_highPerformanceGpuHint_h_56esr4hgrt6
