#ifndef BUFFER_H_xwsertzui
#define BUFFER_H_xwsertzui

#include <cstdlib>
#include <cstring>

namespace melown
{
    class Buffer
    {
    public:
        Buffer() : data(nullptr), size(0)
        {}

        Buffer(uint32 size) : data(nullptr), size(size)
        {
            data = malloc(size);
        }

        Buffer(const Buffer &other) : data(nullptr), size(other.size)
        {
            data = malloc(size);
            memcpy(data, other.data, size);
        }

        Buffer(Buffer &&other) : data(other.data), size(other.size)
        {
            other.data = nullptr;
            other.size = 0;
        }

        ~Buffer()
        {
            free(data);
            data = nullptr;
        }

        Buffer &operator = (const Buffer &other)
        {
            if (&other == this)
                return *this;
            free(data);
            size = other.size;
            data = malloc(size);
            memcpy(data, other.data, size);
        }

        Buffer &operator = (Buffer &&other)
        {
            if (&other == this)
                return *this;
            free(data);
            size = other.size;
            data = other.data;
            other.data = nullptr;
            other.size = 0;
        }

        void *data;
        uint32 size;
    };
}

#endif
