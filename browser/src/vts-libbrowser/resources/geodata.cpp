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

#include <dbglog/dbglog.hpp>

#include "../geodata.hpp"
#include "../fetchTask.hpp"
#include "../renderTasks.hpp"
#include "../map.hpp"

namespace vts
{

GeodataFeatures::GeodataFeatures(vts::MapImpl *map, const std::string &name) :
    Resource(map, name)
{}

void GeodataFeatures::load()
{
    LOG(info2) << "Loading geodata features <" << name << ">";
    data = std::make_shared<const std::string>(
        std::move(fetch->reply.content.str()));
}

FetchTask::ResourceType GeodataFeatures::resourceType() const
{
    return FetchTask::ResourceType::GeodataFeatures;
}

GeodataStylesheet::GeodataStylesheet(MapImpl *map, const std::string &name) :
    Resource(map, name)
{
    priority = std::numeric_limits<float>::infinity();
}

void GeodataStylesheet::load()
{
    LOG(info2) << "Loading geodata stylesheet <" << name << ">";
    data = std::make_shared<const std::string>(
        std::move(fetch->reply.content.str()));
}

FetchTask::ResourceType GeodataStylesheet::resourceType() const
{
    return FetchTask::ResourceType::GeodataStylesheet;
}

GeodataTile::GeodataTile(MapImpl *map, const std::string &name)
    : Resource(map, name), lod(0)
{
    state = Resource::State::ready;
}

FetchTask::ResourceType GeodataTile::resourceType() const
{
    return FetchTask::ResourceType::Undefined;
}

void GeodataTile::update(const std::shared_ptr<const std::string> &s,
    const std::shared_ptr<const std::string> &f, uint32 l)
{
    switch ((Resource::State)state)
    {
    case Resource::State::initializing:
        state = Resource::State::availFail; // if left in initializing, it would attempt to download it
        UTILITY_FALLTHROUGH;
    case Resource::State::availFail:
    case Resource::State::errorFatal: // allow reloading when sources change, even if it failed before
    case Resource::State::ready:
        if (style != s || features != f || lod != l)
        {
            style = s;
            features = f;
            lod = l;
            state = Resource::State::downloading;
            map->resources.queGeodata.push(
                std::dynamic_pointer_cast<GeodataTile>(shared_from_this()));
            return;
        }
        break;
    default:
        // nothing
        break;
    }
}

GpuGeodataSpec::GpuGeodataSpec()
{
    matToRaw(identityMatrix4(), model);
}

GpuGeodataSpec::UnionData::UnionData()
{
    memset(this, 0, sizeof(*this));
}

GpuGeodataSpec::CommonData::CommonData() :
visibility(nan1()), culling(nan1()), zIndex(0)
{
    vecToRaw(vec4f(nan4().cast<float>()), visibilityRelative);
    vecToRaw(vec3f(nan3().cast<float>()), zBufferOffset);
    vecToRaw(vec2f(nan2().cast<float>()), visibilityAbsolute);
}

GpuMeshSpec GpuGeodataSpec::createMeshPoints() const
{
    uint32 linesCount = coordinates.size();
    uint32 totalVertices = 0;
    for (uint32 li = 0; li < linesCount; li++)
        totalVertices += coordinates[li].size();
    GpuMeshSpec spec;
    spec.verticesCount = totalVertices;
    spec.vertices.allocate(spec.verticesCount * sizeof(vec3f));
    vec3f *out = (vec3f*)spec.vertices.data();
    for (uint32 li = 0; li < linesCount; li++)
    {
        memcpy(out, coordinates[li].data(),
                coordinates[li].size() * sizeof(vec3f));
        out += coordinates[li].size();
    }
    spec.attributes[0].enable = true;
    spec.attributes[0].components = 3;
    spec.attributes[0].type = GpuTypeEnum::Float;
    spec.faceMode = GpuMeshSpec::FaceMode::Points;
    return spec;
}

} // namespace vts
