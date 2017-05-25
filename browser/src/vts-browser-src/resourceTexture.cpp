#include "map.hpp"
#include "image.hpp"

namespace vts
{

GpuTextureSpec::GpuTextureSpec() : width(0), height(0), components(0)
{}

GpuTextureSpec::GpuTextureSpec(const Buffer &buffer)
{
    decodeImage(buffer, this->buffer, width, height, components);
}

void GpuTextureSpec::verticalFlip()
{
    unsigned lineSize = width * components;
    Buffer tmp(lineSize);
    for (unsigned y = 0; y < height / 2; y++)
    {
        char *a = buffer.data() + y * lineSize;
        char *b = buffer.data()
                + (height - y - 1) * lineSize;
        memcpy(tmp.data(), a, lineSize);
        memcpy(a, b, lineSize);
        memcpy(b, tmp.data(), lineSize);
    }
}

void GpuTexture::load()
{
    LOG(info2) << "Loading gpu texture '" << fetch->name << "'";
    GpuTextureSpec spec(fetch->contentData);
    spec.verticalFlip();
    fetch->map->callbacks.loadTexture(info, spec);
    info.ramMemoryCost += sizeof(*this);
}

} // namespace vts
