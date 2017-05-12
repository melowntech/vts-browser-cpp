#include <geometry/parse-obj.hpp>
#include <vts/math.hpp>

#include "obj.hpp"

namespace vts
{

void decodeObj(const Buffer &in, uint32 &outFaceMode,
               Buffer &outVertices, Buffer &,
               uint32 &vertices, uint32 &indices)
{
    geometry::Obj obj;
    detail::Wrapper w(in);
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
        
        /* this triggers a warning in g++
        for (uint32 i = 1; i < outFaceMode; i++)
        {
            for (uint32 j = 0; j < i; j++)
                if (of.v[i] == of.v[j])
                    outFaceMode--;
        }
        */
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
