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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#include <windows.h>
#endif // _WIN32

#include <cstdlib>

#include <dbglog/dbglog.hpp>

#include "../include/vts-browser/foundation.hpp"
#include "../utilities/detectMeasurementSystem.hpp"

namespace vts
{

namespace
{

uint32 parseEnv(const char *env)
{
    const char *l = env;
    const char *p = env;
    while (*p)
    {
        // find a sequence of alpha characters
        if ((*p >= 'a' && *p <= 'z')
         || (*p >= 'A' && *p <= 'Z'))
        {
            p++;
            continue;
        }
        if (l == p)
        {
            l = ++p;
            continue;
        }
        // is it US (case-insensitive)?
        if (p == l + 2 && (l[0] == 'u' || l[0] == 'U')
                && (l[1] == 's' || l[1] == 'S'))
            return 0; // US
        l = p;
    }
    if (p == l + 2 && (l[0] == 'u' || l[0] == 'U')
            && (l[1] == 's' || l[1] == 'S'))
        return 0; // US
    return 1; // metric
}

uint32 detectMeasurementSystemImpl()
{
#ifdef _WIN32
    // todo GetLocaleInfoEx
#endif // _WIN32

    if (auto p = std::getenv("LC_MEASUREMENT"))
        return parseEnv(p);
    if (auto p = std::getenv("LANG"))
        return parseEnv(p);
    if (auto p = std::getenv("LANGUAGE"))
        return parseEnv(p);
    return 1; // metric
}

uint32 detectMeasurementSystemLog()
{
    uint32 v = detectMeasurementSystemImpl();
    LOG(info2) << (v ? "Detected metric measurement units system"
                     : "Detected US measurement units system");
    return v;
}

} // namespace

uint32 detectMeasurementSystem()
{
    static uint32 value = detectMeasurementSystemLog();
    return value;
}

} // namespace vts

