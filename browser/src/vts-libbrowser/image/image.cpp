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

#include <cstdio>
#include <dbglog/dbglog.hpp>
#include "image.hpp"

namespace vts
{

void decodeImage(const Buffer &in, Buffer &out,
                 uint32 &width, uint32 &height, uint32 &components)
{
    if (in.size() < 8)
        LOGTHROW(err1, std::runtime_error) << "insufficient image data";
    static const unsigned char pngSignature[]
            = { 137, 80, 78, 71, 13, 10, 26, 10 };
    static const unsigned char jpegSignature[]
        = { 0xFF, 0xD8, 0xFF };
    if (memcmp(in.data(), pngSignature, sizeof(pngSignature)) == 0)
        decodePng(in, out, width, height, components);
    else if (memcmp(in.data(), jpegSignature, sizeof(jpegSignature)) == 0)
        decodeJpeg(in, out, width, height, components);
    else
    {
        out = in.copy();
        width = height = components = 0;
    }
}

} // namespace vts
