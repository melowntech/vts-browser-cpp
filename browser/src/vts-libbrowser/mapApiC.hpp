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

#ifndef MAPAPIC_HPP_sdrg456de6
#define MAPAPIC_HPP_sdrg456de6

#include "include/vts-browser/foundation.h"

#include <string>
#include <memory>

#define C_BEGIN try {
#define C_END } catch(...) { vts::handleExceptions(); }

namespace vts
{

VTS_API void handleExceptions();
VTS_API void setError(sint32 code, const std::string &msg);
VTS_API const char *retStr(const std::string &str);

class Map;
class Camera;

} // namespace

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtsCMap
{
    std::shared_ptr<vts::Map> p;
    void *userData = nullptr;
} vtsCMap;

typedef struct vtsCCamera
{
    std::shared_ptr<vts::Camera> p;
} vtsCCamera;

#ifdef __cplusplus
} // extern C
#endif

#endif
