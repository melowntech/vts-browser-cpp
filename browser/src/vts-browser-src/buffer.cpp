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

#include <cstring>
#include <map>
#include <boost/filesystem.hpp>
#include <dbglog/dbglog.hpp>

#include "include/vts-browser/buffer.hpp"

std::map<std::string, std::pair<size_t, unsigned char *>> data_map;

namespace vts
{

Buffer::Buffer() : data_(nullptr), size_(0)
{}

Buffer::Buffer(uint32 size) : data_(nullptr), size_(size)
{
    allocate(size);
}

Buffer::Buffer(const Buffer &other) : data_(nullptr), size_(other.size_)
{
    allocate(size_);
    memcpy(data_, other.data_, size_);
}

Buffer::Buffer(Buffer &&other) : data_(other.data_), size_(other.size_)
{
    other.data_ = nullptr;
    other.size_ = 0;
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
    allocate(other.size_);
    memcpy(data_, other.data_, size_);
    return *this;
}

Buffer &Buffer::operator = (Buffer &&other)
{
    if (&other == this)
        return *this;
    this->free();
    size_ = other.size_;
    data_ = other.data_;
    other.data_ = nullptr;
    other.size_ = 0;
    return *this;
}

void Buffer::allocate(uint32 size)
{
    this->free();
    this->size_ = size;
    data_ = (char*)malloc(size_);
    if (!data_)
        LOGTHROW(err2, std::runtime_error) << "not enough memory";
}

void Buffer::free()
{
    ::free(data_);
    data_ = nullptr;
    size_ = 0;
}

void writeLocalFileBuffer(const std::string &path, const Buffer &buffer)
{
    boost::filesystem::create_directories(
                boost::filesystem::path(path).parent_path());
    FILE *f = fopen(path.c_str(), "wb");
    if (!f)
        LOGTHROW(err1, std::runtime_error) << "failed to write file";
    if (fwrite(buffer.data(), buffer.size(), 1, f) != 1)
    {
        fclose(f);
        LOGTHROW(err1, std::runtime_error) << "failed to write file";
    }
    if (fclose(f) != 0)
        LOGTHROW(err1, std::runtime_error) << "failed to write file";
}

Buffer readLocalFileBuffer(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        LOGTHROW(err1, std::runtime_error) << "failed to read local file";
    try
    {
        fseek(f, 0, SEEK_END);
        Buffer b(ftell(f));
        fseek(f, 0, SEEK_SET);
        if (fread(b.data(), b.size(), 1, f) != 1)
            LOGTHROW(err1, std::runtime_error)
                    << "failed to read local file";
        fclose(f);
        return b;
    }
    catch (...)
    {
        fclose(f);
        throw;
    }
}

Buffer readInternalMemoryBuffer(const std::string &path)
{
    auto it = data_map.find(path);
    if (it == data_map.end())
        LOGTHROW(err1, std::runtime_error) << "internal resource not found";
    Buffer b(it->second.first);
    memcpy(b.data(), it->second.second, b.size());
    return b;
}

namespace detail
{

Wrapper::Wrapper(const Buffer &b) : std::istream(this)
{
    setg(b.data(), b.data(), b.data() + b.size());
    clear();
}

} // namespace

} // namespace vts
