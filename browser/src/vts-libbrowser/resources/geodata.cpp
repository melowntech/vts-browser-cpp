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
    data = std::move(fetch->reply.content.str());
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
    data = std::move(fetch->reply.content.str());
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

void GeodataTile::update(const std::string &s, const std::string &f, uint32 l)
{
    switch ((Resource::State)state)
    {
    case Resource::State::initializing:
        state = Resource::State::ready; // if left in initializing, it would attempt to download it
        UTILITY_FALLTHROUGH;
    case Resource::State::errorFatal: // allow reloading when sources change, even if it failed before
    case Resource::State::ready:
        if (style != s || features != f || lod != l)
        {
            style = s;
            features = f;
            lod = l;
            state = Resource::State::downloaded;
            map->resources.queUpload.push(shared_from_this());
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
    GpuMeshSpec spec;
    spec.verticesCount = coordinates.size();
    spec.vertices.allocate(spec.verticesCount * sizeof(vec3f));
    memcpy(spec.vertices.data(), coordinates.data(), spec.vertices.size());
    spec.attributes[0].enable = true;
    spec.attributes[0].components = 3;
    spec.attributes[0].type = GpuTypeEnum::Float;
    spec.faceMode = GpuMeshSpec::FaceMode::Points;
    return spec;
}

GpuMeshSpec GpuGeodataSpec::createMeshLines() const
{
    GpuMeshSpec spec;
    spec.verticesCount = coordinates.size();
    spec.vertices.allocate(spec.verticesCount * sizeof(vec3f));
    memcpy(spec.vertices.data(), coordinates.data(), spec.vertices.size());
    spec.attributes[0].enable = true;
    spec.attributes[0].components = 3;
    spec.attributes[0].type = GpuTypeEnum::Float;
    spec.faceMode = GpuMeshSpec::FaceMode::Lines;
    return spec;
}

namespace
{

vec3f localUp(const mat4 &model, const mat4 &modelInv, const vec3f &p)
{
    vec3 dl = p.cast<double>();
    vec3 dw = vec4to3(vec4(model * vec3to4(dl, 1)));
    vec3 upw = normalize(dw);
    vec3 tw = dw + upw;
    vec3 tl = vec4to3(vec4(modelInv * vec3to4(tw, 1)));
    return (tl - dl).cast<float>();
}


GpuMeshSpec createMeshLineWorld(const GpuGeodataSpec &geo)
{
    uint32 linesCount = geo.coordinates.size() / 2;
    uint32 trianglesCount = linesCount * 2;

    std::vector<vec3f> vertices;
    vertices.reserve(trianglesCount * 3);
    std::vector<uint16> indices;
    indices.reserve(trianglesCount * 3);

    const mat4 model = rawToMat4(geo.model);
    const mat4 modelInv = model.inverse();

    for (uint32 li = 0; li < linesCount; li++)
    {
        std::array<float, 3> aa = geo.coordinates[li * 2 + 0];
        std::array<float, 3> bb = geo.coordinates[li * 2 + 1];
        vec3f a(aa[0], aa[1], aa[2]), b(bb[0], bb[1], bb[2]);
        if (a == b)
            continue;
        vec3f c = (a + b) * 0.5;
        vec3f up = localUp(model, modelInv, c);
        float localMeter = length(up);
        up = normalize(up);
        vec3f forward = normalize(vec3f(b - a));
        vec3f side = normalize(cross(forward, up));
        float scale = geo.unionData.line.width * localMeter * 0.5;
        forward *= scale;
        side *= scale;
        uint32 ib = vertices.size();

        /*
               0----------------------------------------------------7    
              /                                                      \   
             /                                                        \  
            1                                                          6 
            |  a                                                    b  | 
            2                                                          5 
             \                                                        /  
              \                                                      /   
               3----------------------------------------------------4    
        */

        vertices.push_back(a - side);
        vertices.push_back(a - forward * 0.7 - side * 0.5);
        vertices.push_back(a - forward * 0.7 + side * 0.5);
        vertices.push_back(a + side);

        vertices.push_back(b + side);
        vertices.push_back(b + forward * 0.7 + side * 0.5);
        vertices.push_back(b + forward * 0.7 - side * 0.5);
        vertices.push_back(b - side);

        indices.push_back(ib + 0);
        indices.push_back(ib + 1);
        indices.push_back(ib + 7);
        indices.push_back(ib + 1);
        indices.push_back(ib + 6);
        indices.push_back(ib + 7);
        indices.push_back(ib + 1);
        indices.push_back(ib + 2);
        indices.push_back(ib + 6);
        indices.push_back(ib + 2);
        indices.push_back(ib + 5);
        indices.push_back(ib + 6);
        indices.push_back(ib + 2);
        indices.push_back(ib + 3);
        indices.push_back(ib + 5);
        indices.push_back(ib + 3);
        indices.push_back(ib + 4);
        indices.push_back(ib + 5);
    }

    GpuMeshSpec spec;
    spec.verticesCount = vertices.size();
    spec.vertices.allocate(vertices.size() * sizeof(vec3f));
    memcpy(spec.vertices.data(), vertices.data(), spec.vertices.size());
    spec.indicesCount = indices.size();
    spec.indices.allocate(indices.size() * sizeof(uint16));
    memcpy(spec.indices.data(), indices.data(), spec.indices.size());
    spec.attributes[0].enable = true;
    spec.attributes[0].components = 3;
    spec.attributes[0].type = GpuTypeEnum::Float;
    spec.faceMode = GpuMeshSpec::FaceMode::Triangles;
    return spec;
}

} // namespace

GpuMeshSpec GpuGeodataSpec::createMesh() const
{
    // todo
    switch (type)
    {
    case Type::LineWorld:
        return createMeshLineWorld(*this);
    case Type::LineLabel:
    case Type::LineScreen:
        return createMeshLines();
    case Type::PointScreen:
    case Type::PointWorld:
    case Type::PointLabel:
    case Type::Icon:
    case Type::PackedPointLabelIcon:
    case Type::Triangles:
        return createMeshPoints();
    }
    return GpuMeshSpec();
}

} // namespace vts
