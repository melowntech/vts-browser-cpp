#include <geometry/parse-obj.hpp>
#include <vts/math.hpp>

#include "obj.hpp"

namespace vts
{

void decodeObj(const Buffer &in, uint32 &outFaceMode,
               Buffer &outVertices, Buffer &,
               uint32 &vertices, uint32 &indices)
{
    Buffer buf(in);
    geometry::Obj obj;
    if (!obj.parse(buf))
        throw std::runtime_error("failed to decode obj file");

    // find face mode
    outFaceMode = 3;
    for (geometry::Obj::Facet &of : obj.facets)
    {
        for (uint32 i = 1; i < outFaceMode; i++)
        {
            for (uint32 j = 0; j < i; j++)
                if (of.v[i] == of.v[j])
                    outFaceMode--;
        }
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
            fs->uvs = vecFromUblas<vec3f>(obj.texcoords[of.t[i]]).head(2);
            fs++;
        }
    }
    vertices = obj.facets.size() * outFaceMode;
    indices = 0;
}

} // namespace vts
