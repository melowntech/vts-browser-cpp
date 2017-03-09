#ifndef BUFFER_H_xwsertzui
#define BUFFER_H_xwsertzui

#include "foundation.h"

namespace melown
{

class Buffer
{
public:
    Buffer();
    Buffer(uint32 size);
    Buffer(const Buffer &other);
    Buffer(Buffer &&other);
    Buffer &operator = (const Buffer &other);
    Buffer &operator = (Buffer &&other);
    ~Buffer();

    void allocate(uint32 size);
    void free();

    void *data;
    uint32 size;
};

} // namespace melown

#endif
