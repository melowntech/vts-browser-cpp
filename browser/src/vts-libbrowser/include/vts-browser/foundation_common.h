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

#ifndef FOUNDATION_COMMON_H_wbhdfh
#define FOUNDATION_COMMON_H_wbhdfh

#ifdef _MSVC
#define VTS_API_EXPORT _declspec(dllexport)
#define VTS_API_IMPORT _declspec(dllimport)
#define VTS_THREAD_LOCAL_STORAGE __declspec(thread)
#else
#define VTS_API_EXPORT __attribute__((visibility ("default")))
#define VTS_API_IMPORT __attribute__((visibility ("default")))
#define VTS_THREAD_LOCAL_STORAGE __thread
#endif

#ifdef VTS_BROWSER_BUILD_STATIC
#define VTS_API
#elif VTS_BROWSER_BUILD_SHARED
#define VTS_API VTS_API_EXPORT
#else
#define VTS_API VTS_API_IMPORT
#endif

#endif

