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

#ifdef _WIN32
#include <windows.h> // GetCurrentProcessId
#else
#include <unistd.h> // getpid
#endif

#include <boost/filesystem.hpp>
#include <dbglog/dbglog.hpp>

#include "../include/vts-browser/buffer.hpp"

void initializeBrowserData();
namespace
{
    class BrowserDataInitializator
    {
    public:
        BrowserDataInitializator()
        {
            initializeBrowserData();
        }
    } browserDataInitializatorInstance;
} // namespace

namespace vts
{

namespace
{

uint64 getPid()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

static const uint64 pid = getPid();

std::string uniqueName()
{
    static std::atomic<uint32> tmpIndex;
    std::ostringstream ss;
    ss << pid << "_" << (tmpIndex++);
    return ss.str();
}

typedef std::map<std::string, std::pair<size_t, const unsigned char *>>
    dataMapType;

dataMapType &dataMap()
{
    static dataMapType data;
    return data;
}

} // namespace

Buffer::Buffer() : data_(nullptr), size_(0)
{}

Buffer::Buffer(uint32 size) : data_(nullptr), size_(size)
{
    allocate(size);
}

Buffer::Buffer(const std::string &str) : data_(nullptr), size_(0)
{
    allocate(str.length());
    memcpy(data_, str.data(), size_);
}

Buffer::~Buffer()
{
    this->free();
}

Buffer::Buffer(Buffer &&other) noexcept : data_(other.data_), size_(other.size_)
{
    other.data_ = nullptr;
    other.size_ = 0;
}

Buffer &Buffer::operator = (Buffer &&other) noexcept
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

Buffer Buffer::copy() const
{
    Buffer r(size_);
    memcpy(r.data(), data_, size_);
    return r;
}

std::string Buffer::str() const
{
    return std::string(data_, size_);
}

void Buffer::allocate(uint32 size)
{
    this->free();
    this->size_ = size;
    data_ = (char*)malloc(size_);
    if (!data_)
        LOGTHROW(err2, std::runtime_error)
                << "Not enough memory for buffer allocation, requested "
                << size << " bytes";
}

void Buffer::resize(uint32 size)
{
    char *tmp = (char*)realloc(data_, size);
    if (!tmp)
    {
        LOGTHROW(err2, std::runtime_error)
                << "Not enough memory for buffer reallocation, requested "
                << size << " bytes";
    }
    this->size_ = size;
    data_ = tmp;
}

void Buffer::free()
{
    ::free(data_);
    data_ = nullptr;
    size_ = 0;
}

void writeLocalFileBuffer(const std::string &path, const Buffer &buffer)
{
    std::string folderPath = boost::filesystem::path(path)
            .parent_path().string();
    if (!folderPath.empty())
        boost::filesystem::create_directories(folderPath);
    std::string tmpPath = path + "_tmp_" + uniqueName();
    FILE *f = fopen(tmpPath.c_str(), "wb");
    if (!f)
        LOGTHROW(err1, std::runtime_error) << "Failed to write file <"
                                           << path << ">";
    if (buffer.size() > 0)
    {
        if (fwrite(buffer.data(), buffer.size(), 1, f) != 1)
        {
            fclose(f);
            LOGTHROW(err1, std::runtime_error) << "Failed to write file <"
                                               << path << ">";
        }
    }
    if (fclose(f) != 0)
        LOGTHROW(err1, std::runtime_error) << "Failed to write file <"
                                           << path << ">";
    boost::filesystem::rename(tmpPath, path);
}

Buffer readLocalFileBuffer(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        LOGTHROW(err1, std::runtime_error) << "Failed to read file <"
                                           << path << ">";
    try
    {
        fseek(f, 0, SEEK_END);
        Buffer b(ftell(f));
        fseek(f, 0, SEEK_SET);
        if (fread(b.data(), b.size(), 1, f) != 1)
            LOGTHROW(err1, std::runtime_error) << "Failed to read file <"
                                               << path << ">";
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
    auto it = dataMap().find(path);
    if (it == dataMap().end())
        LOGTHROW(err1, std::runtime_error) << "Internal buffer <"
                                           << path << "> not found";
    Buffer b(it->second.first);
    memcpy(b.data(), it->second.second, b.size());
    return b;
}

namespace detail
{

BufferStream::BufferStream(const Buffer &b) : std::istream(this)
{
    setg(b.data(), b.data(), b.data() + b.size());
    exceptions(std::istream::badbit | std::istream::failbit);
}

uint32 BufferStream::position() const
{
    return gptr() - eback();
}

void addInternalMemoryData(const std::string name,
                           const unsigned char *data, size_t size)
{
    assert(!existsInternalMemoryData(name));
    dataMap()[name] = std::make_pair(size, data);
}

bool existsInternalMemoryData(const std::string &path)
{
    return dataMap().find(path) != dataMap().end();
}

} // namespace detail

} // namespace vts
