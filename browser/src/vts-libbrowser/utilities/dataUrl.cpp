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

#include "dataUrl.hpp"

#include "utility/base64.hpp"
#include "utility/uri.hpp"

#include <dbglog/dbglog.hpp>

namespace vts
{

void readDataUrl(const std::string &url,
    Buffer &buff, std::string &contentType)
{
    const auto comma = url.find(',', 5);
    if (comma == std::string::npos)
    {
        LOGTHROW(err1, std::runtime_error)
            << "No comma in data: url.";
    }

    bool base64 = false;
    // find last semicolon from comma and match the segment base64 marker
    const auto semi(url.rfind(';', comma));
    if (semi != std::string::npos)
        base64 = !url.compare(semi, comma - semi, ";base64");

    if (base64)
        contentType = url.substr(5, semi - 5);
    else
        contentType = url.substr(5, comma - 5);

    buff = Buffer(base64
        ? utility::base64::decode(url.begin() + comma + 1, url.end())
        : utility::urlDecode(url.begin() + comma + 1, url.end()));
}

} // namespace vts

