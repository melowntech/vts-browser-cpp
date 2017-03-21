#ifndef OBJ_H_wrtzeubfnjk
#define OBJ_H_wrtzeubfnjk

#include <string>

#include <renderer/buffer.h>

namespace melown
{

void decodeObj(const std::string &name, Buffer &in,
               Buffer &outVertices, Buffer &outIndices,
               uint32 &vertices, uint32 &indices);

} // namespace melown

#endif
