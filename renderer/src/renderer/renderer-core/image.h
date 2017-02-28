#ifndef IMAGE_H_erweubdnu
#define IMAGE_H_erweubdnu

#include <stdexcept>
#include <string>

#include <renderer/foundation.h>
#include "buffer.h"

namespace melown
{
    typedef std::runtime_error imageDecodeException;

    void decodeImage(const std::string &name, const Buffer &in, Buffer &out, uint32 &width, uint32 &height, uint32 &components);
}

#endif
