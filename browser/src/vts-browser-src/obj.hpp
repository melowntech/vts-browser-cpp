#ifndef OBJ_H_wrtzeubfnjk
#define OBJ_H_wrtzeubfnjk

#include <string>

#include "include/vts-browser/buffer.hpp"

namespace vts
{

void decodeObj(const Buffer &in, uint32 &outFaceMode,
               Buffer &outVertices, Buffer &outIndices,
               uint32 &vertices, uint32 &indices);

} // namespace vts

#endif
