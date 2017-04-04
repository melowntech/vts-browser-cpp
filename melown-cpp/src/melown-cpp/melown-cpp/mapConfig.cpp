#include <utility/uri.hpp>
#include <boost/algorithm/string.hpp>

#include "resource.hpp"
#include "mapConfig.hpp"
#include "color.hpp"

namespace melown
{

const std::string convertPath(const std::string &path,
                              const std::string &parent)
{
    return utility::Uri(parent).resolve(path).str();
}

SurfaceInfo::SurfaceInfo(const SurfaceCommonConfig &surface,
                         const std::string &parentPath)
    : SurfaceCommonConfig(surface)
{
    urlMeta.parse(convertPath(urls3d->meta, parentPath));
    urlMesh.parse(convertPath(urls3d->mesh, parentPath));
    urlIntTex.parse(convertPath(urls3d->texture, parentPath));
    urlNav.parse(convertPath(urls3d->nav, parentPath));
}

BoundInfo::BoundInfo(const BoundLayer &layer) : BoundLayer(layer)
{
    urlExtTex.parse(url);
    if (metaUrl)
    {
        urlMeta.parse(*metaUrl);
        urlMask.parse(*maskUrl);
    }
}

SurfaceStackItem::SurfaceStackItem() : alien(false)
{}

MapConfig::MapConfig(const std::string &name) : Resource(name)
{}

void MapConfig::load(MapImpl *)
{
    vtslibs::vts::loadMapConfig(*this, impl->contentData, name);
    
    convertor = std::shared_ptr<CsConvertor>(CsConvertor::create(
                  referenceFrame.model.physicalSrs,
                  referenceFrame.model.navigationSrs,
                  referenceFrame.model.publicSrs,
                  *this
                  ));
    
    generateSurfaceStack();
    
    //LOG(info3) << "position: " << position.position;
    //LOG(info3) << "rotation: " << position.orientation;
}

vtslibs::vts::SurfaceCommonConfig *MapConfig::findGlue(
        const vtslibs::vts::Glue::Id &id)
{
    for (auto &&it : glues)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

vtslibs::vts::SurfaceCommonConfig *MapConfig::findSurface(const std::string &id)
{
    for (auto &&it : surfaces)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

BoundInfo *MapConfig::getBoundInfo(const std::__cxx11::string &id)
{
    auto it = boundInfos.find(id);
    if (it == boundInfos.end())
        return nullptr;
    return it->second.get();
}


void MapConfig::printSurfaceStack()
{
    std::ostringstream ss;
    ss << std::endl;
    for (auto &&l : surfaceStack)
        ss << (l.alien ? "* " : "  ")
           << std::setw(100) << std::left << (std::string()
           + "[" + boost::algorithm::join(l.surface->name, ",") + "]")
           << " " << l.color.transpose() << std::endl;
    LOG(info3) << ss.str();
}

void MapConfig::generateSurfaceStack()
{
    vtslibs::vts::TileSetGlues::list lst;
    for (auto &&s : view.surfaces)
    {
        vtslibs::vts::TileSetGlues ts(s.first);
        for (auto &&g : glues)
        {
            if (g.id.back() == ts.tilesetId)
                ts.glues.push_back(vtslibs::vts::Glue(g.id));
        }
        lst.push_back(ts);
    }
    
    // sort surfaces by their order in mapConfig
    std::unordered_map<std::string, uint32> order;
    uint32 i = 0;
    for (auto &&it : surfaces)
        order[it.id] = i++;
    std::sort(lst.begin(), lst.end(), [order](
              vtslibs::vts::TileSetGlues &a,
              vtslibs::vts::TileSetGlues &b) mutable {
        assert(order.find(a.tilesetId) != order.end());
        assert(order.find(b.tilesetId) != order.end());
        return order[a.tilesetId] < order[b.tilesetId];
    });
    
    // sort glues on surfaces
    lst = vtslibs::vts::glueOrder(lst);
    std::reverse(lst.begin(), lst.end());
    
    // generate proper surface stack
    surfaceStack.clear();
    for (auto &&ts : lst)
    {
        for (auto &&g : ts.glues)
        {
            SurfaceStackItem i;
            i.surface = std::shared_ptr<SurfaceInfo> (new SurfaceInfo(
                    *findGlue(g.id), name));
            i.surface->name = g.id;
            surfaceStack.push_back(i);
        }
        SurfaceStackItem i;
        i.surface = std::shared_ptr<SurfaceInfo> (new SurfaceInfo(
                    *findSurface(ts.tilesetId), name));
        i.surface->name = { ts.tilesetId };
        surfaceStack.push_back(i);
    }
    
    // colorize proper surface stack
    for (auto it = surfaceStack.begin(),
         et = surfaceStack.end(); it != et; it++)
        it->color = convertHsvToRgb(vec3f((it - surfaceStack.begin())
                                          / (float)surfaceStack.size(), 1, 1));
    
    // generate alien surface stack positions
    auto copy(surfaceStack);
    for (auto &&it : copy)
    {
        if (it.surface->name.size() > 1)
        {
            auto n2 = it.surface->name;
            n2.pop_back();
            std::string n = boost::join(n2, "|");
            auto jt = surfaceStack.begin(), et = surfaceStack.end();
            while (jt != et && boost::join(jt->surface->name, "|") != n)
                jt++;
            if (jt != et)
            {
                SurfaceStackItem i(it);
                i.alien = true;
                surfaceStack.insert(jt, i);
            }
        }
    }
    
    // colorize alien positions
    for (auto it = surfaceStack.begin(),
         et = surfaceStack.end(); it != et; it++)
    {
        if (it->alien)
        {
            vec3f c = convertRgbToHsv(it->color);
            c(1) *= 0.5;
            it->color = convertHsvToRgb(c);
        }
    }
    
    // debug print
    //printSurfaceStack();
}

ExternalBoundLayer::ExternalBoundLayer(const std::string &name)
    : Resource(name)
{}

void ExternalBoundLayer::load(MapImpl *)
{
    *(vtslibs::registry::BoundLayer*)this
        = vtslibs::registry::loadBoundLayer(impl->contentData, name);
}

} // namespace melown
