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

#ifndef FOUNDATION_HPP_wefwghefi
#define FOUNDATION_HPP_wefwghefi

#include "foundation_common.h"
#include <vts-browser/foundation.hpp>

#ifndef VTSR_INCLUDE_GL
typedef void *(*GLADloadproc)(const char *name);
#endif

namespace vts { namespace renderer
{

VTSR_API void checkGlImpl(const char *name = "");
VTSR_API void checkGlFramebuffer(uint32 target);

#ifdef NDEBUG
#define CHECK_GL(NAME)
#define CHECK_GL_FRAMEBUFFER(TARGET)
#else
#define CHECK_GL(NAME) checkGlImpl(NAME)
#define CHECK_GL_FRAMEBUFFER(TARGET) checkGlFramebuffer(TARGET)
#endif

// initialize all gl functions
// should be called once after the gl context has been created
VTSR_API void loadGlFunctions(GLADloadproc functionLoader);

} } // namespace vts::renderer

#endif
