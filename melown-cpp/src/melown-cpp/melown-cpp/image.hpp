#ifndef IMAGE_H_erweubdnu
#define IMAGE_H_erweubdnu

#include <string>

#include <melown/buffer.hpp>

namespace melown
{

void decodeImage(const std::string &name, const Buffer &in, Buffer &out,
                 uint32 &width, uint32 &height, uint32 &components);

} // namespace melown

#endif
