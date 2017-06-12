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

#ifndef RESOURCES_H_jhsegfshg
#define RESOURCES_H_jhsegfshg

#include <vector>
#include <memory>

#include "buffer.hpp"

namespace vts
{

class VTS_API ResourceInfo
{
public:
    ResourceInfo();
    
    std::shared_ptr<void> userData;
    uint32 ramMemoryCost;
    uint32 gpuMemoryCost;
};

class VTS_API GpuTextureSpec
{
public:
    GpuTextureSpec();
    GpuTextureSpec(const Buffer &buffer); // decode jpg or png file
    void verticalFlip();
    
    uint32 width, height;
    uint32 components; // 1, 2, 3 or 4
    Buffer buffer;
};

class VTS_API GpuMeshSpec
{
public:
    enum class FaceMode
    { // OpenGL constants
        Points = 0x0000,
        Lines = 0x0001,
        LineStrip = 0x0003,
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        TriangleFan = 0x0006,
    };

    struct VertexAttribute
    {
        enum class Type
        { // OpenGL constants
            Float = 0x1406,
            UnsignedByte = 0x1401,
        };

        VertexAttribute();
        uint32 offset; // in bytes
        uint32 stride; // in bytes
        uint32 components; // 1, 2, 3 or 4
        Type type;
        bool enable;
        bool normalized;
    };

    GpuMeshSpec();
    GpuMeshSpec(const Buffer &buffer); // decode obj file

    Buffer vertices;
    Buffer indices;
    std::vector<VertexAttribute> attributes;
    uint32 verticesCount;
    uint32 indicesCount;
    FaceMode faceMode;
};

} // namespace vts

#endif
