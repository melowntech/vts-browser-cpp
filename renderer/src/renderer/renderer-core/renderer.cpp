#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/optional/optional_io.hpp>
#include <vts-libs/vts/urltemplate.hpp>
#include <vts-libs/vts/nodeinfo.hpp>
#include <dbglog/dbglog.hpp>
#include <utility/uri.hpp>

#include <renderer/gpuResources.h>
#include <renderer/gpuContext.h>
#include <renderer/statistics.h>

#include "map.h"
#include "mapConfig.h"
#include "renderer.h"
#include "resourceManager.h"
#include "mapResources.h"
#include "csConvertor.h"
#include "resource.h"

namespace melown
{

using namespace vtslibs::vts;
using namespace vtslibs::registry;

namespace
{

const vec3f convertRgbToHsv(const vec3f &inColor)
{
    vec3f outColor;
    float minColor = inColor[0] < inColor[1] ? inColor[0] : inColor[1];
    minColor = minColor < inColor[2] ? minColor : inColor[2];
    float maxColor = inColor[0] > inColor[1] ? inColor[0] : inColor[1];
    maxColor = maxColor > inColor[2] ? maxColor : inColor[2];
    outColor[2] = maxColor;
    float delta = maxColor - minColor;
    if (delta < 0.00001)
        return outColor;
    if (maxColor > 0)
        outColor[1] = (delta / maxColor);
    else
        return outColor;
    if (inColor[0] >= maxColor)
        outColor[0] = (inColor[1] - inColor[2]) / delta;
    else
        if (inColor[1] >= maxColor)
            outColor[0] = 2 + (inColor[2] - inColor[0]) / delta;
        else
            outColor[0] = 4 + (inColor[0] - inColor[1]) / delta;
    outColor[0] *= 60.f / 360.f;
    if (outColor[0] < 0)
        outColor[0] += 1;
    return outColor;
}

const vec3f convertHsvToRgb(const vec3f &inColor)
{
    vec3f outColor;
    if (inColor[1] <= 0)
    {
        outColor[0] = inColor[2];
        outColor[1] = inColor[2];
        outColor[2] = inColor[2];
        return outColor;
    }
    float hh = inColor[0];
    if (hh >= 1)
        hh = 0;
    hh /= 60.f / 360.f;
    uint32 i = (uint32)hh;
    float ff = hh - i;
    float p = inColor[2] * (1 - inColor[1]);
    float q = inColor[2] * (1 - (inColor[1] * ff));
    float t = inColor[2] * (1 - (inColor[1] * (1 - ff)));
    switch (i)
    {
    case 0:
        outColor[0] = inColor[2];
        outColor[1] = t;
        outColor[2] = p;
        break;
    case 1:
        outColor[0] = q;
        outColor[1] = inColor[2];
        outColor[2] = p;
        break;
    case 2:
        outColor[0] = p;
        outColor[1] = inColor[2];
        outColor[2] = t;
        break;
    case 3:
        outColor[0] = p;
        outColor[1] = q;
        outColor[2] = inColor[2];
        break;
    case 4:
        outColor[0] = t;
        outColor[1] = p;
        outColor[2] = inColor[2];
        break;
    case 5:
    default:
        outColor[0] = inColor[2];
        outColor[1] = p;
        outColor[2] = q;
        break;
    }
    return outColor;
}

const vec3 lowerUpperCombine(uint32 i)
{
    vec3 res;
    res(0) = (i >> 0) % 2;
    res(1) = (i >> 1) % 2;
    res(2) = (i >> 2) % 2;
    return res;
}

const vec4 column(const mat4 &m, uint32 index)
{
    return vec4(m(index, 0), m(index, 1), m(index, 2), m(index, 3));
}

SurfaceCommonConfig *findGlue(MapConfig *mapConfig, const Glue::Id &id)
{
    for (auto &&it : mapConfig->glues)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

SurfaceCommonConfig *findSurface(MapConfig *mapConfig, const std::string &id)
{
    for (auto &&it : mapConfig->surfaces)
    {
        if (it.id == id)
            return &it;
    }
    return nullptr;
}

}

class RendererImpl : public Renderer
{
public:
    class SurfaceInfo : public SurfaceCommonConfig
    {
    public:
        SurfaceInfo(const SurfaceCommonConfig &surface, RendererImpl *renderer)
            : SurfaceCommonConfig(surface)
        {
            urlMeta.parse(renderer->convertPath(urls3d->meta,
                                      renderer->mapConfig->name));
            urlMesh.parse(renderer->convertPath(urls3d->mesh,
                                      renderer->mapConfig->name));
            urlIntTex.parse(renderer->convertPath(urls3d->texture,
                                        renderer->mapConfig->name));
        }

        UrlTemplate urlMeta;
        UrlTemplate urlMesh;
        UrlTemplate urlIntTex;
        TilesetIdList name;
    };
    
    class SurfaceStackItem
    {
    public:
        SurfaceStackItem() : alien(false)
        {}
        
        std::shared_ptr<SurfaceInfo> surface;
        vec3f color;
        bool alien;
    };

    class BoundInfo : public BoundLayer
    {
    public:
        BoundInfo(const BoundLayer &layer, RendererImpl *impl)
            : BoundLayer(layer), impl(impl)
        {
            urlExtTex.parse(url);
            if (metaUrl)
            {
                urlMeta.parse(*metaUrl);
                urlMask.parse(*maskUrl);
            }
        }

        RendererImpl *impl;
        UrlTemplate urlExtTex;
        UrlTemplate urlMeta;
        UrlTemplate urlMask;
    };

    class BoundParamInfo : public View::BoundLayerParams
    {
    public:
        typedef std::vector<BoundParamInfo> list;

        BoundParamInfo(const View::BoundLayerParams &params)
            : View::BoundLayerParams(params), bound(nullptr),
              watertight(true), availCache(false),
              vars(0), orig(0)
        {}
        
        bool available(bool primaryLod = false)
        {
            if (availCache)
                return true;  
            
            // check mask texture
            if (bound->metaUrl && !watertight)
            {
                if (primaryLod)
                {
                    GpuTexture *t = impl->map->resources->getTexture(
                                bound->urlMask(vars));
                    if (!*t)
                        return false;
                }
                else
                {
                    if (!impl->map->resources->ready(
                                bound->urlMask(vars)))
                        return false;
                }
            }
            
            // check color texture
            if (primaryLod)
            {
                GpuTexture *t = impl->map->resources->getTexture(
                            bound->urlExtTex(vars));
                t->impl->availTest = bound->availability.get_ptr();
                return availCache = *t;
            }
            else
                return availCache = impl->map->resources->ready(
                            bound->urlExtTex(vars));
        }
        
        void varsFallDown(int depth)
        {
            if (depth == 0)
                return;
            vars.tileId.lod -= depth;
            vars.tileId.x >>= depth;
            vars.tileId.y >>= depth;
            vars.localId.lod -= depth;
            vars.localId.x >>= depth;
            vars.localId.y >>= depth;
        }
        
        const mat3f uvMatrix() const
        {
            int dep = depth();
            if (dep == 0)
                return upperLeftSubMatrix(identityMatrix()).cast<float>();
            double scale = 1.0 / (1 << dep);
            double tx = scale * (orig.localId.x
                                 - ((orig.localId.x >> dep) << dep));
            double ty = scale * (orig.localId.y
                                 - ((orig.localId.y >> dep) << dep));
            ty = 1 - scale - ty;
            mat3f m;
            m << scale, 0, tx,
                 0, scale, ty,
                 0, 0, 1;
            return m;
        }
        
        bool canGoUp() const
        {
            return vars.localId.lod <= bound->lodRange.min + 1;
        }
        
        int depth() const
        {
            return std::max(nodeInfo->nodeId().lod - bound->lodRange.max, 0)
                    + orig.localId.lod - vars.localId.lod;
        }

        void prepare(const NodeInfo &nodeInfo, RendererImpl *impl,
                     uint32 subMeshIndex)
        {
            this->nodeInfo = &nodeInfo;
            this->impl = impl;
            bound = impl->getBoundInfo(id);
            if (!bound)
                return;
            orig = vars = UrlTemplate::Vars(nodeInfo.nodeId(), local(nodeInfo),
                                            subMeshIndex);
            varsFallDown(depth());
            
            if (bound->metaUrl)
            { // bound meta nodes
                UrlTemplate::Vars v(vars);
                v.tileId.x &= ~255;
                v.tileId.y &= ~255;
                v.localId.x &= ~255;
                v.localId.y &= ~255;
                BoundMetaTile *bmt = impl->map->resources->getBoundMetaTile(
                            bound->urlMeta(v));
                if (!*bmt)
                {
                    bound = nullptr;
                    return;
                }
                
                uint8 f = bmt->flags[(vars.tileId.y & 255) * 256
                        + (vars.tileId.x & 255)];
                if ((f & BoundLayer::MetaFlags::available)
                        != BoundLayer::MetaFlags::available)
                {
                    bound = nullptr;
                    return;
                }
                
                watertight = (f & BoundLayer::MetaFlags::watertight)
                        == BoundLayer::MetaFlags::watertight;
            }
        }

        bool invalid() const
        {
            if (!bound)
                return true;
            TileId t = nodeInfo->nodeId();
            int m = bound->lodRange.min;
            if (t.lod < m)
                return true;
            t.x >>= t.lod - m;
            t.y >>= t.lod - m;
            if (t.x < bound->tileRange.ll[0] || t.x > bound->tileRange.ur[0])
                return true;
            if (t.y < bound->tileRange.ll[1] || t.y > bound->tileRange.ur[1])
                return true;
            return false;
        }

        bool operator < (const BoundParamInfo &rhs) const
        {
            return depth() > rhs.depth();
        }
        
        UrlTemplate::Vars orig;
        UrlTemplate::Vars vars;
        const NodeInfo *nodeInfo;
        RendererImpl *impl;
        BoundInfo *bound;
        bool watertight;
        
    private:
        bool availCache;
    };

    RendererImpl(MapImpl *map) : map(map), mapConfig(nullptr),
        shader(nullptr), gpuContext(nullptr)
    {}

    void renderInitialize(GpuContext *gpuContext) override
    {
        this->gpuContext = gpuContext;
    }

    void reorderBoundLayer(const NodeInfo &nodeInfo, uint32 subMeshIndex,
            BoundParamInfo::list &boundList)
    {
        for (BoundParamInfo &b : boundList)
            b.prepare(nodeInfo, this, subMeshIndex);
        
        // erase invalid layers
        boundList.erase(std::remove_if(boundList.begin(), boundList.end(),
            [](BoundParamInfo &a){ return a.invalid(); }),
                        boundList.end());
        if (boundList.empty())
            return;
            
        // sort by depth and priority
        std::stable_sort(boundList.begin(), boundList.end());
        std::reverse(boundList.begin(), boundList.end()); // render in reverse
        
        // skip overlapping layers
        auto it = boundList.begin(), et = boundList.end();
        while (it != et && !it->watertight)
            it++;
        if (it != et)
            boundList.erase(++it, et);
        
        // show lower resolution when appropriate
        for (BoundParamInfo &b : boundList)
        {
            for (uint32  i = 0; i < 5; i++)
            {
                if (b.available(i == 0) || !b.canGoUp())
                    break;
                b.varsFallDown(1);
            }
        }
        
        // erase not-ready layers
        boundList.erase(std::remove_if(boundList.begin(), boundList.end(),
            [](BoundParamInfo &a){ return !a.available(); }),
                        boundList.end());
    }

    void drawBox(const mat4f &mvp, const vec3f &color = vec3f(1, 0, 0))
    {
        GpuMeshRenderable *mesh
                = map->resources->getMeshRenderable("data/cube.obj");
        if (!*mesh)
            return;

        shaderColor->bind();
        shaderColor->uniformMat4(0, mvp.data());
        shaderColor->uniformVec3(8, color.data());
        gpuContext->wiremode(true);
        mesh->draw();
        gpuContext->wiremode(false);
        shader->bind();
    }

    bool renderNode(const NodeInfo &nodeInfo,
                    const SurfaceStackItem &surface,
                    bool onlyCheckReady)
    {
        // nodeId
        const TileId nodeId = nodeInfo.nodeId();

        // statistics
        if (!onlyCheckReady)
        {
            map->statistics->meshesRenderedTotal++;
            map->statistics->meshesRenderedPerLod[
                    std::min<uint32>(nodeId.lod, MapStatistics::MaxLods-1)]++;
        }

        // aggregate mesh
        MeshAggregate *meshAgg = map->resources->getMeshAggregate(surface
                .surface->urlMesh(UrlTemplate::Vars(nodeId, local(nodeInfo))));
        if (!*meshAgg)
            return false;

        // iterate over all submeshes
        for (uint32 subMeshIndex = 0, e = meshAgg->submeshes.size();
             subMeshIndex != e; subMeshIndex++)
        {
            const MeshPart &part = meshAgg->submeshes[subMeshIndex];
            GpuMeshRenderable *mesh = part.renderable.get();
            if (!onlyCheckReady)
            {
                mat4f mvp = (viewProj * part.normToPhys).cast<float>();
                shader->uniformMat4(0, mvp.data());
                //drawBox(mvp, surface.color);
            }
            
            // internal texture
            if (part.internalUv)
            {
                UrlTemplate::Vars vars(nodeId, local(nodeInfo), subMeshIndex);
                GpuTexture *texture = map->resources->getTexture(
                            surface.surface->urlIntTex(vars));
                if (onlyCheckReady)
                {
                    if (!*texture)
                        return false;
                }
                else
                {
                    if (!*texture)
                        continue;
                    texture->bind();
                    mat3f uvm = upperLeftSubMatrix(identityMatrix())
                            .cast<float>();
                    shader->uniformMat3(4, uvm.data());
                    shader->uniform(8, 0); // internal uv
                    mesh->draw();
                }
                continue;
            }

            // external bound textures
            if (part.externalUv)
            {
                std::string surfaceName;
                if (surface.surface->name.size() > 1)
                    surfaceName = surface.surface
                            ->name[part.surfaceReference - 1];
                else
                    surfaceName = surface.surface->name.back();
                const View::BoundLayerParams::list &boundList
                                = mapConfig->view.surfaces[surfaceName];
                BoundParamInfo::list bls(boundList.begin(), boundList.end());
                if (part.textureLayer)
                    bls.push_back(BoundParamInfo(View::BoundLayerParams(
                        mapConfig->boundLayers.get(part.textureLayer).id)));
                reorderBoundLayer(nodeInfo, subMeshIndex, bls);
                if (onlyCheckReady && bls.empty())
                    return false;
                if (!onlyCheckReady)
                for (BoundParamInfo &b : bls)
                {
                    if (!b.watertight)
                    {
                        gpuContext->activeTextureUnit(1);
                        map->resources->getTexture(b.bound->urlMask(b.vars))
                                ->bind();
                        gpuContext->activeTextureUnit(0);
                        shader->uniform(9, 1); // use mask
                    }
                    map->resources->getTexture(b.bound->urlExtTex(b.vars))
                            ->bind();
                    mat3f uvm = b.uvMatrix();
                    shader->uniformMat3(4, uvm.data());
                    shader->uniform(8, 1); // external uv
                    mesh->draw();
                    if (!b.watertight)
                        shader->uniform(9, 0); // do not use mask
                }
            }
        }
        
        return true;
    }
    
    /*
    bool checkParentsChild(const SurfaceInfo *surface, const TileId &id)
    {
        if (id.lod == 0)
            return false;
        TileId pid = parent(id);
        const TileId tileId(pid.lod,
                            (pid.x >> binaryOrder) << binaryOrder,
                            (pid.y >> binaryOrder) << binaryOrder);
        const MetaTile *pt = map->resources->getMetaTile(surface->urlMeta(
                                            UrlTemplate::Vars(tileId)));
        if (!*pt)
            return false;
        const MetaNode *pn = pt->get(pid, std::nothrow);
        if (!pn)
            return false;
        uint32 idx = 0;
        idx += (id.y - pid.y * 2) * 2;
        idx += id.x - pid.x * 2;
        assert(idx < 4);
        return (pn->flags() & (MetaNode::Flag::ulChild << idx)) != 0;
    }
    */

    const bool visibilityTest(const NodeInfo &nodeInfo, const MetaNode &node)
    {
        vec4 c0 = column(viewProj, 0);
        vec4 c1 = column(viewProj, 1);
        vec4 c2 = column(viewProj, 2);
        vec4 c3 = column(viewProj, 3);
        vec4 planes[6] = {
            c3 + c0, c3 - c0,
            c3 + c1, c3 - c1,
            c3 + c2, c3 - c2,
        };
        vec3 fl = vecFromUblas<vec3>(node.extents.ll);
        vec3 fu = vecFromUblas<vec3>(node.extents.ur);
        vec3 el = vecFromUblas<vec3>
          (mapConfig->referenceFrame.division.extents.ll);
        vec3 eu = vecFromUblas<vec3>
          (mapConfig->referenceFrame.division.extents.ur);
        vec3 box[2] = {
            fl.cwiseProduct(eu - el) + el,
            fu.cwiseProduct(eu - el) + el,
        };
        for (uint32 i = 0; i < 6; i++)
        {
            vec4 &p = planes[i]; // current plane
            vec3 pv = vec3( // current p-vertex
                box[!!(p[0] > 0)](0),
                box[!!(p[1] > 0)](1),
                box[!!(p[2] > 0)](2)
            );
            double d = dot(vec4to3(p), pv);
            if (d < -p[3])
                return false;
        }
        return true;
    }

    bool coarsenessTest(const NodeInfo &nodeInfo, const MetaNode &node)
    {
        if (!node.applyTexelSize() && !node.applyDisplaySize())
            return false;
        bool result = true;
        if (node.applyTexelSize())
        {
            vec3 fl = vecFromUblas<vec3>(node.extents.ll);
            vec3 fu = vecFromUblas<vec3>(node.extents.ur);
            vec3 el = vecFromUblas<vec3>
                    (mapConfig->referenceFrame.division.extents.ll);
            vec3 eu = vecFromUblas<vec3>
                    (mapConfig->referenceFrame.division.extents.ur);
            vec3 up = perpendicularUnitVector * node.texelSize;
            for (uint32 i = 0; i < 8; i++)
            {
                vec3 f = lowerUpperCombine(i).cwiseProduct(fu - fl) + fl;
                vec3 c1 = f.cwiseProduct(eu - el) + el - up * 0.5;
                vec3 c2 = c1 + up;
                c1 = vec4to3(viewProj * vec3to4(c1, 1), true);
                c2 = vec4to3(viewProj * vec3to4(c2, 1), true);
                double len = length(vec3(c2 - c1)) * windowHeight;
                result = result && len < 1.2;
            }
        }
        if (node.applyDisplaySize())
        {
            result = false;
        }
        return result;
    }
    
    void traverse(const NodeInfo &nodeInfo)
    {
        const TileId nodeId = nodeInfo.nodeId();

        // statistics
        map->statistics->metaNodesTraversedTotal++;
        map->statistics->metaNodesTraversedPerLod[
                std::min<uint32>(nodeId.lod, MapStatistics::MaxLods-1)]++;
        
        // vars
        const SurfaceStackItem *topmost = nullptr;
        const MetaNode *node = nullptr;
        bool childsAvailable[4] = {false, false, false, false};

        // find topmost nonempty surface
        for (auto &&it : surfaceStack)
        {
            const TileId tileId(nodeId.lod,
                                (nodeId.x >> binaryOrder) << binaryOrder,
                                (nodeId.y >> binaryOrder) << binaryOrder);
            const MetaTile *t = map->resources->getMetaTile(
                        it.surface->urlMeta(UrlTemplate::Vars(tileId)));
            if (!*t)
                continue;
            const MetaNode *n = t->get(nodeInfo.nodeId(), std::nothrow);
            if (!n || n->extents.ll == n->extents.ur)
                continue;
            if (!visibilityTest(nodeInfo, *n))
                continue;
            for (uint32 i = 0; i < 4; i++)
                childsAvailable[i] = childsAvailable[i]
                        || (n->childFlags() & (MetaNode::Flag::ulChild << i));
            if (topmost || n->alien() != it.alien || !n->geometry())
                continue;
            topmost = &it;
            node = n;
        }
        
        bool anychild = false;
        bool allchild = true;
        for (uint32 i = 0; i < 4; i++)
        {
            anychild = anychild || childsAvailable[i];
            allchild = allchild && childsAvailable[i];
        }
        
        // render the node, if available
        if (topmost && (!allchild || coarsenessTest(nodeInfo, *node)))
        {
            renderNode(nodeInfo, *topmost, false);
            return;
        }
        
        // traverse children
        Children childs = children(nodeId);
        for (uint32 i = 0; i < childs.size(); i++)
            if (childsAvailable[i])
                traverse(nodeInfo.child(childs[i]));
    }

    void updateCamera()
    {
        // todo position conversions
        if (mapConfig->position.type
                != vtslibs::registry::Position::Type::objective)
            throw std::invalid_argument("unsupported position type");
        if (mapConfig->position.heightMode
                != vtslibs::registry::Position::HeightMode::fixed)
            throw std::invalid_argument("unsupported position height mode");

        vec3 center = vecFromUblas<vec3>(mapConfig->position.position);
        vec3 rot = vecFromUblas<vec3>(mapConfig->position.orientation);

        vec3 dir(1, 0, 0);
        vec3 up(0, 0, -1);

        { // apply rotation
            mat3 tmp = upperLeftSubMatrix(rotationMatrix(2, degToRad(-rot(0))))
                    * upperLeftSubMatrix(rotationMatrix(1, degToRad(-rot(1))));
            dir = tmp * dir;
            up = tmp * up;
        }

        switch (mapConfig->srs.get
                (mapConfig->referenceFrame.model.navigationSrs).type)
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            // swap XY
            std::swap(dir(0), dir(1));
            std::swap(up(0), up(1));
            // invert Z
            dir(2) *= -1;
            up(2) *= -1;
            // add center of orbit (transform to navigation srs)
            dir += center;
            up += center;
            // transform to physical srs
            center = map->convertor->navToPhys(center);
            dir = map->convertor->navToPhys(dir);
            up = map->convertor->navToPhys(up);
            // points -> vectors
            dir = normalize(dir - center);
            up = normalize(up - center);
        } break;
        case vtslibs::registry::Srs::Type::geographic:
        {
            // find lat-lon coordinates of points moved to north and east
            vec3 n2 = map->convertor->navGeodesicDirect(center, 0, 100);
            vec3 e2 = map->convertor->navGeodesicDirect(center, 90, 100);
            // transform to physical srs
            center = map->convertor->navToPhys(center);
            vec3 n = map->convertor->navToPhys(n2);
            vec3 e = map->convertor->navToPhys(e2);
            // points -> vectors
            n = normalize(n - center);
            e = normalize(e - center);
            // construct NED coordinate system
            vec3 d = normalize(cross(n, e));
            e = normalize(cross(n, d));
            mat3 tmp = (mat3() << n, e, d).finished();
            // rotate original vectors
            dir = tmp * dir;
            up = tmp * up;
            dir = normalize(dir);
            up = normalize(up);
        } break;
        default:
            throw std::invalid_argument("not implemented navigation srs type");
        }

        double dist = mapConfig->position.verticalExtent
                * 0.5 / tan(degToRad(mapConfig->position.verticalFov * 0.5));
        mat4 view = lookAt(center - dir * dist, center, up);
        mat4 proj = perspectiveMatrix(mapConfig->position.verticalFov,
                        (double)windowWidth / (double)windowHeight,
                        dist * 0.1, dist * 10);
        viewProj = proj * view;
        perpendicularUnitVector = normalize(cross(up, dir));
    }

    const std::string convertPath(const std::string &path,
                                  const std::string &parent)
    {
        return utility::Uri(parent).resolve(path).str();
    }

    const bool configLoaded()
    {
        mapConfig = map->resources->getMapConfig(map->mapConfigPath);
        if (!mapConfig || !*mapConfig)
            return false;

        bool ok = true;
        for (auto &&bl : mapConfig->boundLayers)
        {
            if (!bl.external())
                continue;
            std::string url = convertPath(bl.url, mapConfig->name);
            ExternalBoundLayer *r
                    = map->resources->getExternalBoundLayer(url);
            if (!*r)
                ok = false;
            else
            {
                r->id = bl.id;
                r->url = convertPath(r->url, url);
                if (r->metaUrl)
                    r->metaUrl = convertPath(*r->metaUrl, url);
                if (r->maskUrl)
                    r->maskUrl = convertPath(*r->maskUrl, url);
                if (r->creditsUrl)
                    r->creditsUrl = convertPath(*r->creditsUrl, url);
                mapConfig->boundLayers.replace(*r);
            }
        }
        return ok;
    }
    
    void printSurfaceStack()
    {
        std::ostringstream ss;
        ss << std::endl;
        for (auto &&l : surfaceStack)
            ss << (l.alien ? "* " : "  ")
                << std::setw(50) << std::left << (std::string()
                + "[" + boost::algorithm::join(l.surface->name, ",") + "]")
                << " " << l.color.transpose() << std::endl;
        LOG(info3) << ss.str();
    }
    
    void generateSurfaceStack()
    {
        TileSetGlues::list lst;
        for (auto &&s : mapConfig->view.surfaces)
        {
            TileSetGlues ts(s.first);
            for (auto &&g : mapConfig->glues)
            {
                if (g.id.back() == ts.tilesetId)
                    ts.glues.push_back(Glue(g.id));
            }
            lst.push_back(ts);
        }
        
        // sort surfaces by their order in mapConfig
        std::unordered_map<std::string, uint32> order;
        uint32 i = 0;
        for (auto &&it : mapConfig->surfaces)
            order[it.id] = i++;
        std::sort(lst.begin(), lst.end(), [order](
                  TileSetGlues &a,
                  TileSetGlues &b) mutable {
            assert(order.find(a.tilesetId) != order.end());
            assert(order.find(b.tilesetId) != order.end());
            return order[a.tilesetId] < order[b.tilesetId];
        });
        
        // sort glues on surfaces
        lst = glueOrder(lst);
        std::reverse(lst.begin(), lst.end());
        
        // generate proper surface stack
        surfaceStack.clear();
        for (auto &&ts : lst)
        {
            for (auto &&g : ts.glues)
            {
                SurfaceStackItem i;
                i.surface = std::shared_ptr<SurfaceInfo> (new SurfaceInfo(
                            *findGlue(mapConfig, g.id), this));
                i.surface->name = g.id;
                surfaceStack.push_back(i);
            }
            SurfaceStackItem i;
            i.surface = std::shared_ptr<SurfaceInfo> (new SurfaceInfo(
                    *findSurface(mapConfig, ts.tilesetId), this));
            i.surface->name = {ts.tilesetId};
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
        printSurfaceStack();
    }

    void onceInitialize()
    {
        map->convertor = std::shared_ptr<CsConvertor>(CsConvertor::create(
            mapConfig->referenceFrame.model.physicalSrs,
            mapConfig->referenceFrame.model.navigationSrs,
            mapConfig->referenceFrame.model.publicSrs,
            *mapConfig
        ));

        binaryOrder = mapConfig->referenceFrame.metaBinaryOrder;

        boundInfos.clear();
        for (auto &&bl : mapConfig->boundLayers)
        {
            boundInfos[bl.id] = std::shared_ptr<BoundInfo>
                    (new BoundInfo(bl, this));
        }
        
        generateSurfaceStack();

        /*
        // todo - temporarily reset camera
        //mapConfig->position.orientation = math::Point3(0, -90, 0);
        // todo - temporarily reset position height
        if (mapConfig->position.heightMode == Position::HeightMode::floating)
        {
            mapConfig->position.heightMode = Position::HeightMode::fixed;
            mapConfig->position.position[0] = 471776.0;
            mapConfig->position.position[1] = 5555744.0;
            mapConfig->position.position[2] = 250;
            mapConfig->position.verticalExtent = 1000;
        }
        */
        // temporarily change height above terrain
        mapConfig->position.position[2] = 0;

        LOG(info3) << "position: " << mapConfig->position.position;
        LOG(info3) << "rotation: " << mapConfig->position.orientation;

        /*
        for (auto &&bl : mapConfig->boundLayers)
        {
            LOG(info3) << bl.type;
            LOG(info3) << bl.id;
            LOG(info3) << bl.url;
            LOG(info3) << bl.metaUrl;
            LOG(info3) << bl.maskUrl;
            LOG(info3) << bl.creditsUrl;
        }
        */
    }

    void renderTick(uint32 windowWidth, uint32 windowHeight) override
    {
        if (!configLoaded())
            return;

        this->windowWidth = windowWidth;
        this->windowHeight = windowHeight;

        shaderColor = map->resources->getShader("data/shaders/color.glsl");
        if (!*shaderColor)
            return;

        shader = map->resources->getShader("data/shaders/a.glsl");
        if (!*shader)
            return;
        shader->bind();

        if (!map->convertor)
            onceInitialize();

        updateCamera();

        NodeInfo nodeInfo(mapConfig->referenceFrame, TileId(),
                          false, *mapConfig);
        traverse(nodeInfo);

        map->statistics->frameIndex++;
    }

    void renderFinalize() override
    {}

    BoundInfo *getBoundInfo(const std::string &id)
    {
        auto it = boundInfos.find(id);
        if (it == boundInfos.end())
            return nullptr;
        return it->second.get();
    }

    std::unordered_map<std::string, std::shared_ptr<BoundInfo>> boundInfos;
    std::vector<SurfaceStackItem> surfaceStack;

    vec3 perpendicularUnitVector;
    mat4 viewProj;
    uint32 windowWidth;
    uint32 windowHeight;
    MapImpl *map;
    MapConfig *mapConfig;
    GpuShader *shader;
    GpuShader *shaderColor;
    GpuContext *gpuContext;
    uint32 binaryOrder;
};

Renderer::Renderer()
{}

Renderer::~Renderer()
{}

Renderer *Renderer::create(MapImpl *map)
{
    return new RendererImpl(map);
}

} // namespace melown
