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

#ifndef SEARCH_H_ebhjgsdui
#define SEARCH_H_ebhjgsdui

#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

VTS_API bool vtsMapGetSearchable(vtsHMap map);
VTS_API vtsHSearch vtsMapSearch(vtsHMap map, const char *query);
VTS_API vtsHSearch vtsMapSearchAt(vtsHMap map, const char *query,
                                  const double point[3]);
VTS_API void vtsSearchDestroy(vtsHSearch search);

VTS_API bool vtsSearchGetDone(vtsHSearch search);
VTS_API uint32 vtsSearchGetResultsCount(vtsHSearch search);
VTS_API const char *vtsSearchGetResultData(vtsHSearch search,
                                        uint32 index);
VTS_API void vtsSearchUpdateDistances(vtsHSearch search,
                        const double point[3]);

#ifdef __cplusplus
} // extern C
#endif

#endif
