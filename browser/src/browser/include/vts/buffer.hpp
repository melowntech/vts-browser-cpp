#ifndef BUFFER_H_xwsertzui
#define BUFFER_H_xwsertzui

#include <iostream>
#include <string>

#include "foundation.hpp"

namespace vts
{

class VTS_API Buffer : public std::istream, protected std::streambuf
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

    char *data() const { return data_; }
    uint32 size() const { return size_; }
    
private:
    char *data_;
    uint32 size_;
};

VTS_API Buffer readLocalFileBuffer(const std::string &path);
VTS_API Buffer readInternalMemoryBuffer(const std::string &path);

} // namespace vts

#endif
