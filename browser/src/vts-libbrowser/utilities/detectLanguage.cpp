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
#include <winapifamily.h>
#endif // _WIN32

#include <cstdlib>

#include <dbglog/dbglog.hpp>
#include <utility/getenv.hpp>

#include "../include/vts-browser/foundation.hpp"
#include "detectLanguage.hpp"

namespace vts
{

namespace
{

uint32 detectMeasurementSystemEnv(const char *env)
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

    WCHAR buff[2];
    buff[0] = 0;
    int len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT,
        LOCALE_IMEASURE, buff, 2);
    if (len == 2)
        return buff[0] == '0';

#endif // _WIN32

    if (auto p = utility::getenv("LC_MEASUREMENT"))
        return detectMeasurementSystemEnv(p);
    if (auto p = utility::getenv("LANG"))
        return detectMeasurementSystemEnv(p);
    if (auto p = utility::getenv("LANGUAGE"))
        return detectMeasurementSystemEnv(p);
    return 1; // metric
}

uint32 detectMeasurementSystemLog()
{
    uint32 v = detectMeasurementSystemImpl();
    LOG(info2) << (v ? "Detected metric measurement units system"
                     : "Detected US measurement units system");
    return v;
}

std::string detectLanguageEnv(const char *env)
{
    // todo strip .UTF-8 or similar suffixes
    return env;
}

std::string detectLanguageImpl()
{
#ifdef _WIN32
#if WINAPI_PARTITION_DESKTOP

    ULONG numlangs = 1;
    WCHAR buff[100];
    ULONG buffLen = 99;
    BOOL res = GetUserPreferredUILanguages(MUI_LANGUAGE_NAME,
        &numlangs, buff, &buffLen);
    if (res == TRUE)
    {
        char output[200];
        sprintf(output, "%ws", buff);
        return output;
    }

#endif // WINAPI_PARTITION_DESKTOP
#endif // _WIN32

    if (auto p = utility::getenv("LANG"))
        return detectLanguageEnv(p);
    if (auto p = utility::getenv("LANGUAGE"))
        return detectLanguageEnv(p);
    return "en-US";
}

std::string detectLanguageLog()
{
    std::string v = detectLanguageImpl();
    LOG(info2) << "Detected language: " << v;
    return v;
}

} // namespace

uint32 detectMeasurementSystem()
{
    static const uint32 value = detectMeasurementSystemLog();
    return value;
}

std::string detectLanguage()
{
    static const std::string value = detectLanguageLog();
    return value;
}

} // namespace vts

