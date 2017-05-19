#include <vts-libs/vts/meshio.hpp>

#include "map.hpp"
#include "image.hpp"

namespace vts
{

MetaTile::MetaTile() : 
    vtslibs::vts::MetaTile(vtslibs::vts::TileId(), 0)
{}

void MetaTile::load()
{
    LOG(info2) << "Loading meta tile '" << impl->name << "'";
    detail::Wrapper w(impl->contentData);
    *(vtslibs::vts::MetaTile*)this
            = vtslibs::vts::loadMetaTile(w, 5, impl->name);
    info.ramMemoryCost = this->size() * sizeof(vtslibs::vts::MetaNode);
}

void NavTile::load()
{
    LOG(info2) << "Loading navigation tile '" << impl->name << "'";
    GpuTextureSpec spec;
    decodeImage(impl->contentData, spec.buffer,
                spec.width, spec.height, spec.components);
    if (spec.width != 256 || spec.height != 256 || spec.components != 1)
        LOGTHROW(err1, std::runtime_error) << "invalid navtile image";
    data.resize(256 * 256);
    memcpy(data.data(), spec.buffer.data(), 256 * 256);
    info.ramMemoryCost = data.size();
}

vec2 NavTile::sds2px(const vec2 &point, const math::Extents2 &extents)
{
    return vecFromUblas<vec2>(vtslibs::vts::NavTile::sds2px(
                                  vecToUblas<math::Point2>(point), extents));
}

void BoundMetaTile::load()
{
    LOG(info2) << "Loading bound meta tile '" << impl->name << "'";
    
    Buffer buffer = std::move(impl->contentData);
    GpuTextureSpec spec;
    decodeImage(buffer, spec.buffer,
                spec.width, spec.height, spec.components);
    if (spec.buffer.size() != sizeof(flags))
        LOGTHROW(err1, std::runtime_error)
                << "bound meta tile has invalid resolution";
    memcpy(flags, spec.buffer.data(), spec.buffer.size());
    info.ramMemoryCost = spec.buffer.size();
}

void BoundMaskTile::load()
{
    LOG(info2) << "Loading bound mask tile '" << impl->name << "'";
    
    if (!texture)
    {
        texture = std::make_shared<GpuTexture>();
    }
    
    Buffer buffer = std::move(impl->contentData);
    GpuTextureSpec spec;
    decodeImage(buffer, spec.buffer,
                spec.width, spec.height, spec.components);
    impl->map->callbacks.loadTexture(info, spec);
    info.gpuMemoryCost = texture->info.gpuMemoryCost;
    info.ramMemoryCost = texture->info.ramMemoryCost;
}

} // namespace vts
