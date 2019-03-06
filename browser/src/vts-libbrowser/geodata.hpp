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

#ifndef GEODATA_HPP_o84d6
#define GEODATA_HPP_o84d6

#include "include/vts-browser/math.hpp"
#include "resource.hpp"

namespace vts
{

enum class Validity;

class GpuFont;
class GpuTexture;
class GpuGeodataSpec;
class RenderGeodataTask;

class GeodataFeatures : public Resource
{
public:
    GeodataFeatures(MapImpl *map, const std::string &name);
    void load() override;
    FetchTask::ResourceType resourceType() const override;

    std::shared_ptr<const std::string> data;
};

class GeodataStylesheet : public Resource
{
public:
    GeodataStylesheet(MapImpl *map, const std::string &name);
    void load() override;
    Validity dependencies();
    FetchTask::ResourceType resourceType() const override;

    std::string data;
    std::map<std::string, std::shared_ptr<GpuFont>> fonts;
    std::map<std::string, std::shared_ptr<GpuTexture>> bitmaps;
    Validity dependenciesValidity;
    bool dependenciesLoaded;
};

class GeodataTile : public Resource
{
public:
    GeodataTile(MapImpl *map, const std::string &name);
    void load() override;
    FetchTask::ResourceType resourceType() const override;
    void update(const std::shared_ptr<GeodataStylesheet> &style,
                const std::shared_ptr<const std::string> &features,
                const vec3 aabbPhys[2], uint32 lod);
    void process();

    std::vector<RenderGeodataTask> renders;
    std::vector<GpuGeodataSpec> specsToUpload;

private:
    std::shared_ptr<GeodataStylesheet> style;
    std::shared_ptr<const std::string> features;
    vec3 aabbPhys[2];
    uint32 lod;
};

} // namespace vts

#endif
