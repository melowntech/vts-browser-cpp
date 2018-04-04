/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BUFFER_HPP_xwsertzui
#define BUFFER_HPP_xwsertzui

#include <iostream>
#include <string>

#include "foundation.hpp"

namespace vts
{

class VTS_API Buffer
{
public:
    Buffer();
    explicit Buffer(uint32 size); // create preallocated buffer (it is not zeroed)
    explicit Buffer(const std::string &str); // create buffer from string
    ~Buffer();

    // move semantics
    Buffer(Buffer &&other) noexcept;
    Buffer &operator = (Buffer &&other) noexcept;

    // implicit copy semantics are disabled (for performance reasons)
    Buffer(const Buffer &other) = delete;
    Buffer &operator = (const Buffer &other) = delete;

    // explicitly create a copy
    Buffer copy() const;

    // explicitly create string out of the buffer
    std::string str() const;

    // allocates a new buffer of the specified size
    //   any previous content is lost
    //   the buffer is not zeroed
    void allocate(uint32 size);

    // allocates a new buffer of the specified size
    //   all previous content (up to the size of the new buffer) is preserved
    //   any new space is not zeroed
    // this may invalidate all pointers
    void resize(uint32 size);

    void free();

    char *data() const { return data_; }
    uint32 size() const { return size_; }

private:
    char *data_;
    uint32 size_;
};

VTS_API void writeLocalFileBuffer(const std::string &path,
                                  const Buffer &buffer);
VTS_API Buffer readLocalFileBuffer(const std::string &path);
VTS_API Buffer readInternalMemoryBuffer(const std::string &path);

namespace detail
{

// a convenient buffer wrapper that allows to treat the buffer as a stream
// the buffer must outlive the wrapper and must not be modified
class VTS_API Wrapper : protected std::streambuf, public std::istream
{
public:
    Wrapper(const Buffer &b);
    uint32 position() const;
};

VTS_API void addInternalMemoryData(const std::string name,
                                   const unsigned char *data, size_t size);
VTS_API bool existsInternalMemoryBuffer(const std::string &path);

} // detail

} // namespace vts

#endif
