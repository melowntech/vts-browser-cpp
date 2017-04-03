#include <cstring>

#include <melown/buffer.h>

namespace melown
{

Buffer::Buffer() : data_(nullptr), size_(0), std::istream(this)
{}

Buffer::Buffer(uint32 size) : data_(nullptr), size_(size), std::istream(this)
{
    allocate(size);
}

Buffer::Buffer(const Buffer &other) : data_(nullptr), size_(other.size_),
    std::istream(this)
{
    allocate(size_);
    memcpy(data_, other.data_, size_);
}

Buffer::Buffer(Buffer &&other) : data_(other.data_), size_(other.size_),
    std::istream(this)
{
    other.data_ = nullptr;
    other.size_ = 0;
    setg(data_, data_, data_ + size_);
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
    setg(data_, data_, data_ + size_);
}

void Buffer::allocate(uint32 size)
{
    this->free();
    this->size_ = size;
    data_ = (char*)malloc(size_);
    if (!data_)
        throw std::runtime_error("not enough memory");
    setg(data_, data_, data_ + size_);
}

void Buffer::free()
{
    setg(nullptr, nullptr, nullptr);
    ::free(data_);
    data_ = nullptr;
    size_ = 0;
}

Buffer readLocalFileBuffer(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        throw std::runtime_error("failed to read file");
    try
    {
        fseek(f, 0, SEEK_END);
        Buffer b(ftell(f));
        fseek(f, 0, SEEK_SET);
        if (fread(b.data(), b.size(), 1, f) != 1)
            throw std::runtime_error("failed to read file");
        fclose(f);
        return b;
    }
    catch (...)
    {
        fclose(f);
        throw;
    }
}

} // namespace melown
