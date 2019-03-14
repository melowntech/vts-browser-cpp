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

#include "../include/vts-browser/foundation.hpp"
#include "case.hpp"
#include "case/lower.hpp"
#include "case/title.hpp"
#include "case/upper.hpp"

#include <utf8.h>

namespace vts
{

void concatenate(std::string &r, const uint32 *v, uint32 c)
{
    if (v)
    {
        char a[30];
        const uint32 *e = v;
        while (*e)
            e++;
        auto b = utf8::utf32to8(v, e, a);
        r += std::string(a, b);
    }
    else
    {
        uint32 a[2] = { c, 0 };
        concatenate(r, a, 0);
    }
}

bool isWhitespace(uint32 v)
{
    if (v >= 9 && v <= 13)
        return true;
    if (v >= 8192 && v <= 8202)
        return true;
    switch (v)
    {
    case 32:
    case 133:
    case 160:
    case 5760:
    case 8232:
    case 8233:
    case 8239:
    case 8287:
    case 12288:
    case 6158:
    case 8203:
    case 8204:
    case 8205:
    case 8288:
    case 65279:
        return true;
    default:
        return false;
    }
}

std::string lowercase(const std::string &s)
{
    std::string r;
    r.reserve(s.length() + 10);
    auto it = s.begin();
    const auto e = s.end();
    while (it != e)
    {
        uint32 c = utf8::next(it, e);
        concatenate(r, unicodeLowerCase(c), c);
    }
    return r;
}

std::string uppercase(const std::string &s)
{
    std::string r;
    r.reserve(s.length() + 10);
    auto it = s.begin();
    const auto e = s.end();
    while (it != e)
    {
        uint32 c = utf8::next(it, e);
        concatenate(r, unicodeUpperCase(c), c);
    }
    return r;
}

std::string titlecase(const std::string &s)
{
    bool white = true;
    std::string r;
    r.reserve(s.length() + 10);
    auto it = s.begin();
    const auto e = s.end();
    while (it != e)
    {
        uint32 c = utf8::next(it, e);
        if (white)
            concatenate(r, unicodeTitleCase(c), c);
        else
            concatenate(r, unicodeLowerCase(c), c);
        white = isWhitespace(c);
    }
    return r;
}

} // namespace vts
