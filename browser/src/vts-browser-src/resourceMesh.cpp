#include "map.hpp"
#include "obj.hpp"

namespace vts
{

GpuMeshSpec::GpuMeshSpec() : verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles)
{}

GpuMeshSpec::GpuMeshSpec(const Buffer &buffer) :
    verticesCount(0), indicesCount(0),
    faceMode(FaceMode::Triangles)
{
    uint32 dummy;
    uint32 fm;
    decodeObj(buffer, fm, vertices, indices, verticesCount, dummy);
    switch (fm)
    {
    case 1:
        faceMode = FaceMode::Points;
        break;
    case 2:
        faceMode = FaceMode::Lines;
        break;
    case 3:
        faceMode = FaceMode::Triangles;
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid face mode";
    }
}

GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0),
    components(0), type(Type::Float), enable(false), normalized(false)
{}

void GpuMesh::load()
{
    LOG(info2) << "Loading gpu mesh '" << fetch->name << "'";
    GpuMeshSpec spec(fetch->contentData);
    spec.attributes.resize(3);
    spec.attributes[0].enable = true;
    spec.attributes[0].stride = sizeof(vec3f) + sizeof(vec2f);
    spec.attributes[0].components = 3;
    spec.attributes[1].enable = true;
    spec.attributes[1].stride = spec.attributes[0].stride;
    spec.attributes[1].components = 2;
    spec.attributes[1].offset = sizeof(vec3f);
    spec.attributes[2] = spec.attributes[1];
    fetch->map->callbacks.loadMesh(info, spec);
    info.ramMemoryCost += sizeof(*this);
}

MeshPart::MeshPart() : textureLayer(0), surfaceReference(0),
    internalUv(false), externalUv(false)
{}

namespace {

const mat4 findNormToPhys(const math::Extents3 &extents)
{
    vec3 u = vecFromUblas<vec3>(extents.ur);
    vec3 l = vecFromUblas<vec3>(extents.ll);
    vec3 d = (u - l) * 0.5;
    vec3 c = (u + l) * 0.5;
    mat4 sc = scaleMatrix(d(0), d(1), d(2));
    mat4 tr = translationMatrix(c);
    return tr * sc;
}

} // namespace

void MeshAggregate::load()
{
    LOG(info2) << "Loading (aggregated) mesh '" << fetch->name << "'";
    
    detail::Wrapper w(fetch->contentData);
    vtslibs::vts::NormalizedSubMesh::list meshes = vtslibs::vts::
            loadMeshProperNormalized(w, fetch->name);

    submeshes.clear();
    submeshes.reserve(meshes.size());

    for (uint32 mi = 0, me = meshes.size(); mi != me; mi++)
    {
        vtslibs::vts::SubMesh &m = meshes[mi].submesh;

        char tmp[10];
        sprintf(tmp, "%d", mi);
        std::shared_ptr<GpuMesh> gm = std::make_shared<GpuMesh>();
        gm->fetch = std::make_shared<FetchTaskImpl>(fetch->map,
                    std::string(fetch->name) + "#" + tmp,
                    FetchTask::ResourceType::MeshPart);

        uint32 vertexSize = sizeof(vec3f);
        if (m.tc.size())
            vertexSize += sizeof(vec2f);
        if (m.etc.size())
            vertexSize += sizeof(vec2f);

        GpuMeshSpec spec;
        spec.attributes.resize(3);
        spec.verticesCount = m.faces.size() * 3;
        spec.vertices.allocate(spec.verticesCount * vertexSize);
        uint32 offset = 0;

        { // vertices
            spec.attributes[0].enable = true;
            spec.attributes[0].components = 3;
            vec3f *b = (vec3f*)spec.vertices.data();
            for (vtslibs::vts::Point3u32 f : m.faces)
            {
                for (uint32 j = 0; j < 3; j++)
                {
                    vec3 p3 = vecFromUblas<vec3>(m.vertices[f[j]]);
                    *b++ = p3.cast<float>();
                }
            }
            offset += m.faces.size() * sizeof(vec3f) * 3;
        }

        if (!m.tc.empty())
        { // internal, separated
            spec.attributes[1].enable = true;
            spec.attributes[1].components = 2;
            spec.attributes[1].offset = offset;
            vec2f *b = (vec2f*)(spec.vertices.data() + offset);
            for (vtslibs::vts::Point3u32 f : m.facesTc)
                for (uint32 j = 0; j < 3; j++)
                    *b++ = vecFromUblas<vec2f>(m.tc[f[j]]);
            offset += m.faces.size() * sizeof(vec2f) * 3;
        }

        if (!m.etc.empty())
        { // external, interleaved
            spec.attributes[2].enable = true;
            spec.attributes[2].components = 2;
            spec.attributes[2].offset = offset;
            vec2f *b = (vec2f*)(spec.vertices.data() + offset);
            for (vtslibs::vts::Point3u32 f : m.faces)
                for (uint32 j = 0; j < 3; j++)
                    *b++ = vecFromUblas<vec2f>(m.etc[f[j]]);
            offset += m.faces.size() * sizeof(vec2f) * 3;
        }

        fetch->map->callbacks.loadMesh(gm->info, spec);
        gm->fetch->state = FetchTaskImpl::State::ready;

        MeshPart part;
        part.renderable = gm;
        part.normToPhys = findNormToPhys(meshes[mi].extents) 
                * scaleMatrix(1.001);
        part.internalUv = spec.attributes[1].enable;
        part.externalUv = spec.attributes[2].enable;
        part.textureLayer = m.textureLayer ? *m.textureLayer : 0;
        part.surfaceReference = m.surfaceReference;
        submeshes.push_back(part);
    }

    info.ramMemoryCost += sizeof(*this) + meshes.size() * sizeof(MeshPart);
    for (auto &&it : submeshes)
    {
        info.gpuMemoryCost += it.renderable->info.gpuMemoryCost;
        info.ramMemoryCost += it.renderable->info.ramMemoryCost;
    }
}

} // namespace vts
