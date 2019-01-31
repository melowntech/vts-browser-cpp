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

#ifndef GPURESOURCES_HPP_m1nbr6779
#define GPURESOURCES_HPP_m1nbr6779

#include "include/vts-browser/math.hpp"
#include "resource.hpp"

namespace vts
{

class GpuMesh : public Resource
{
public:
    GpuMesh(MapImpl *map, const std::string &name);
    void load() override;
    FetchTask::ResourceType resourceType() const override;
};

class GpuTexture : public Resource
{
public:
    GpuTexture(MapImpl *map, const std::string &name);
    void load() override;
    FetchTask::ResourceType resourceType() const override;
    GpuTextureSpec::FilterMode filterMode;
    GpuTextureSpec::WrapMode wrapMode;
};

class GpuAtmosphereDensityTexture : public GpuTexture
{
public:
    GpuAtmosphereDensityTexture(MapImpl *map, const std::string &name);
    void load() override;
};

class GpuFont : public Resource
{
public:
    GpuFont(MapImpl *map, const std::string &name);
    void load() override;
    FetchTask::ResourceType resourceType() const override;
};

class GpuGeodata
{
public:
    ResourceInfo info;
};

class MeshPart
{
public:
    MeshPart();
    std::shared_ptr<GpuMesh> renderable;
    mat4 normToPhys;
    uint32 textureLayer;
    uint32 surfaceReference;
    bool internalUv;
    bool externalUv;
};

class MeshAggregate : public Resource
{
public:
    MeshAggregate(MapImpl *map, const std::string &name);
    void load() override;
    FetchTask::ResourceType resourceType() const override;

    std::vector<MeshPart> submeshes;
};

} // namespace vts

#endif
