#ifndef OBJ_H_wrtzeubfnjk
#define OBJ_H_wrtzeubfnjk

#include <stdexcept>
#include <string>

#include <renderer/foundation.h>
#include <renderer/buffer.h>

namespace melown
{

void decodeObj(const std::string &name, const Buffer &in,
               Buffer &outVertices, Buffer &outIndices,
               uint32 &vertices, uint32 &indices);

} // namespace melown

#endif
