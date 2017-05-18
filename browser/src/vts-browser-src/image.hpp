#ifndef IMAGE_H_erweubdnu
#define IMAGE_H_erweubdnu

#include <string>
#include "include/vts-browser/buffer.hpp"

namespace vts
{

void decodeImage(const Buffer &in, Buffer &out,
                 uint32 &width, uint32 &height, uint32 &components);

} // namespace vts

#endif
