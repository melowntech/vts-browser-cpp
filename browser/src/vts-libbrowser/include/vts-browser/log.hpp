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

#ifndef LOG_HPP_eubgfruebhjgfnm
#define LOG_HPP_eubgfruebhjgfnm

#include <string>
#include <functional>

#include "foundation.hpp"

namespace vts
{

enum class LogLevel
{
    all =      0xfffffu,
    none =     0x00000u,
    debug =    0x0000fu,
    info1 =    0x000f0u,
    info2 =    0x00070u,
    info3 =    0x00030u,
    info4 =    0x00010u,
    warn1 =    0x00f00u,
    warn2 =    0x00700u,
    warn3 =    0x00300u,
    warn4 =    0x00100u,
    err1 =     0x0f000u,
    err2 =     0x07000u,
    err3 =     0x03000u,
    err4 =     0x01000u,
    fatal =    0xf0000u,
    default_ = info3 | warn2 | err2,
    verbose =  info2 | warn2 | err2,
    noDebug =  info1 | warn1 | err1,
};

VTS_API void setLogMask(const std::string &mask);
VTS_API void setLogMask(LogLevel mask);
VTS_API void setLogConsole(bool enable);
VTS_API void setLogFile(const std::string &filename);
VTS_API void setLogThreadName(const std::string &name);
VTS_API void addLogSink(LogLevel mask,
                        std::function<void(const std::string&)> callback);
VTS_API void clearLogSinks();
VTS_API void log(LogLevel level, const std::string &message);

} // namespace vts

#endif
