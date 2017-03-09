#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <renderer/buffer.h>

namespace melown
{

Buffer::Buffer() : data(nullptr), size(0)
{}

Buffer::Buffer(uint32 size) : data(nullptr), size(size)
{
    allocate(size);
}

Buffer::Buffer(const Buffer &other) : data(nullptr), size(other.size)
{
    allocate(size);
    memcpy(data, other.data, size);
}

Buffer::Buffer(Buffer &&other) : data(other.data), size(other.size)
{
    other.data = nullptr;
    other.size = 0;
}

Buffer::~Buffer()
{
    this->free();
}

Buffer &Buffer::operator = (const Buffer &other)
{
    if (&other == this)
        return *this;
    this->free();
    allocate(other.size);
    memcpy(data, other.data, size);
}

Buffer &Buffer::operator = (Buffer &&other)
{
    if (&other == this)
        return *this;
    this->free();
    size = other.size;
    data = other.data;
    other.data = nullptr;
    other.size = 0;
}

void Buffer::allocate(uint32 size)
{
    this->free();
    this->size = size;
    data = malloc(size);
    if (!data)
        throw std::runtime_error("not enough memory");
}

void Buffer::free()
{
    ::free(data);
    data = nullptr;
    size = 0;
}

} // namespace melown
