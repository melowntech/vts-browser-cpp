#ifndef OBJ_H_wrtzeubfnjk
#define OBJ_H_wrtzeubfnjk

#include <stdexcept>
#include <string>

#include <renderer/foundation.h>

#include "buffer.h"

namespace melown
{

typedef std::runtime_error objDecodeException;

void decodeObj(const std::string &name, const Buffer &in,
               Buffer &outVertices, Buffer &outIndices,
               uint32 &vertices, uint32 &indices);

} // namespace melown

#endif
