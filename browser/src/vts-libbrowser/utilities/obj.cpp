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

#include "../include/vts-browser/math.hpp"
#include "obj.hpp"

#include <geometry/parse-obj.hpp>

namespace vts
{

void decodeObj(const Buffer &in, uint32 &outFaceMode,
               Buffer &outVertices, Buffer &,
               uint32 &vertices, uint32 &indices)
{
    geometry::Obj obj;
    detail::BufferStream w(in);
    if (!obj.parse(w))
    {
        LOGTHROW(err1, std::runtime_error) << "failed to decode obj file";
    }

    // find face mode
    outFaceMode = 3;
    for (geometry::Obj::Facet &of : obj.facets)
    {
        int v[3];
        for (uint32 i = 0; i < 3; i++)
            v[i] = of.v[i];
        std::sort(v, v + 3);
        uint32 j = std::unique(v, v + 3) - v;
        outFaceMode = std::min(outFaceMode, j);
    }

    struct F
    {
        vec3f vertex;
        vec2f uvs;
    };
    outVertices = Buffer(obj.facets.size() * sizeof(F) * outFaceMode);
    F *fs = (F*)outVertices.data();
    for (auto &of : obj.facets)
    {
        for (uint32 i = 0; i < outFaceMode; i++)
        {
            fs->vertex = vecFromUblas<vec3f>(obj.vertices[of.v[i]]);
            if (of.t[i] >= 0)
                fs->uvs = vecFromUblas<vec3f>(obj.texcoords[of.t[i]]).head(2);
            else
                fs->uvs = vec2f(0, 0);
            fs++;
        }
    }
    vertices = obj.facets.size() * outFaceMode;
    indices = 0;
}

} // namespace vts
