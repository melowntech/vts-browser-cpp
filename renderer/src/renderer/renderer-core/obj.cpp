#include <geometry/parse-obj.hpp>

#include "obj.h"
#include "math.h"

namespace melown
{

void decodeObj(const std::__cxx11::string &name, const Buffer &in,
               Buffer &outVertices, Buffer &,
               uint32 &vertices, uint32 &indices)
{
    geometry::Obj obj;
    std::istringstream is(std::string((char*)in.data, in.size));
    if (!obj.parse(is))
        throw objDecodeException(std::string("failed to decode obj: ") + name);

    struct F
    {
        vec3f vertex;
        vec2f uvs;
    };
    outVertices = Buffer(obj.facets.size() * sizeof(F) * 3);
    F *fs = (F*)outVertices.data;
    for (auto &of : obj.facets)
    {
        for (uint32 i = 0; i < 3; i++)
        {
            fs->vertex = vecFromUblas<vec3f>(obj.vertices[of.v[i]]);
            fs->uvs = vecFromUblas<vec3f>(obj.texcoords[of.t[i]]).head(2);
            fs++;
        }
    }
    vertices = obj.facets.size() * 3;
    indices = 0;
}

} // namespace melown
