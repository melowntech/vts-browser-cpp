#include "map.h"

namespace melown
{

MapImpl::Renderer::Renderer() : shader(nullptr), shaderColor(nullptr)
{}

const TileId MapImpl::roundId(TileId nodeId)
{
    return TileId (nodeId.lod,
   (nodeId.x >> renderer.metaTileBinaryOrder) << renderer.metaTileBinaryOrder,
   (nodeId.y >> renderer.metaTileBinaryOrder) << renderer.metaTileBinaryOrder);
}

void MapImpl::renderInitialize(GpuContext *gpuContext)
{
    renderContext = gpuContext;
}

void MapImpl::reorderBoundLayer(const NodeInfo &nodeInfo, uint32 subMeshIndex,
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

void MapImpl::drawBox(const mat4f &mvp, const vec3f &color)
{
    GpuMeshRenderable *mesh = getMeshRenderable("data/cube.obj");
    if (!*mesh)
        return;
    
    renderer.shaderColor->bind();
    renderer.shaderColor->uniformMat4(0, mvp.data());
    renderer.shaderColor->uniformVec3(8, color.data());
    renderContext->wiremode(true);
    mesh->draw();
    renderContext->wiremode(false);
    renderer.shader->bind();
}

bool MapImpl::renderNode(const NodeInfo &nodeInfo,
                         const SurfaceStackItem &surface, bool onlyCheckReady)
{
    // nodeId
    const TileId nodeId = nodeInfo.nodeId();
    
    // statistics
    if (!onlyCheckReady)
    {
        statistics.meshesRenderedTotal++;
        statistics.meshesRenderedPerLod[
                std::min<uint32>(nodeId.lod, MapStatistics::MaxLods-1)]++;
    }
    
    // aggregate mesh
    MeshAggregate *meshAgg = getMeshAggregate(
        surface.surface->urlMesh(UrlTemplate::Vars(nodeId, local(nodeInfo))));
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
            mat4f mvp = (renderer.viewProj * part.normToPhys).cast<float>();
            renderer.shader->uniformMat4(0, mvp.data());
            //drawBox(mvp, surface.color);
        }
        
        // internal texture
        if (part.internalUv)
        {
            UrlTemplate::Vars vars(nodeId, local(nodeInfo), subMeshIndex);
            GpuTexture *texture = getTexture(surface.surface->urlIntTex(vars));
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
                renderer.shader->uniformMat3(4, uvm.data());
                renderer.shader->uniform(8, 0); // internal uv
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
            const vtslibs::registry::View::BoundLayerParams::list &boundList
                    = mapConfig->view.surfaces[surfaceName];
            BoundParamInfo::list bls(boundList.begin(), boundList.end());
            if (part.textureLayer)
                bls.push_back(BoundParamInfo(
                        vtslibs::registry::View::BoundLayerParams(
                        mapConfig->boundLayers.get(part.textureLayer).id)));
            reorderBoundLayer(nodeInfo, subMeshIndex, bls);
            if (onlyCheckReady && bls.empty())
                return false;
            if (!onlyCheckReady)
                for (BoundParamInfo &b : bls)
                {
                    if (!b.watertight)
                    {
                        renderContext->activeTextureUnit(1);
                        getTexture(b.bound->urlMask(b.vars))->bind();
                        renderContext->activeTextureUnit(0);
                        renderer.shader->uniform(9, 1); // use mask
                    }
                    getTexture(b.bound->urlExtTex(b.vars))->bind();
                    mat3f uvm = b.uvMatrix();
                    renderer.shader->uniformMat3(4, uvm.data());
                    renderer.shader->uniform(8, 1); // external uv
                    mesh->draw();
                    if (!b.watertight)
                        renderer.shader->uniform(9, 0); // do not use mask
                }
        }
    }
    
    return true;
}

const bool MapImpl::visibilityTest(const NodeInfo &nodeInfo,
                                   const MetaNode &node)
{
    //if (node.geomExtents.z == GeomExtents::ZRange::emptyRange())
    {
        vec4 c0 = column(renderer.viewProj, 0);
        vec4 c1 = column(renderer.viewProj, 1);
        vec4 c2 = column(renderer.viewProj, 2);
        vec4 c3 = column(renderer.viewProj, 3);
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
    //else
    {
        // todo
    }
}

bool MapImpl::coarsenessTest(const NodeInfo &nodeInfo, const MetaNode &node)
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
        vec3 up = renderer.perpendicularUnitVector * node.texelSize;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(fu - fl) + fl;
            vec3 c1 = f.cwiseProduct(eu - el) + el - up * 0.5;
            vec3 c2 = c1 + up;
            c1 = vec4to3(renderer.viewProj * vec3to4(c1, 1), true);
            c2 = vec4to3(renderer.viewProj * vec3to4(c2, 1), true);
            double len = length(vec3(c2 - c1)) * renderer.windowHeight;
            result = result && len < 1.2;
        }
    }
    if (node.applyDisplaySize())
    {
        result = false;
    }
    return result;
}

void MapImpl::traverse(const NodeInfo &nodeInfo)
{
    const TileId nodeId = nodeInfo.nodeId();
    const TileId tileId = roundId(nodeId);
    UrlTemplate::Vars tileVars(tileId);
    
    // statistics
    statistics.metaNodesTraversedTotal++;
    statistics.metaNodesTraversedPerLod[
            std::min<uint32>(nodeId.lod, MapStatistics::MaxLods-1)]++;
    
    // back-tile culling
    if (nodeInfo.distanceFromRoot() < 6 && nodeInfo.distanceFromRoot() > 2)
    {
        vec2 fl = vecFromUblas<vec2>(nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(nodeInfo.extents().ur);
        vec2 c = (fl + fu) * 0.5;
        vec3 an = vec2to3(c, 0);
        vec3 ap = convertor->convert(an, nodeInfo.srs(),
                    mapConfig->referenceFrame.model.physicalSrs);
        vec3 bn = vec2to3(c, 100);
        vec3 bp = convertor->convert(bn, nodeInfo.srs(),
                    mapConfig->referenceFrame.model.physicalSrs);
        vec3 dir = normalize(bp - ap);
        if (dot(dir, renderer.forwardUnitVector) > 0)
            return;
    }
    
    // vars
    const SurfaceStackItem *topmost = nullptr;
    const MetaNode *node = nullptr;
    bool childsAvailable[4] = {false, false, false, false};
    
    // find topmost nonempty surface
    for (auto &&it : mapConfig->surfaceStack)
    {
        const MetaTile *t = getMetaTile(it.surface->urlMeta(tileVars));
        if (!*t)
            continue;
        const MetaNode *n = t->get(nodeId, std::nothrow);
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
    vtslibs::vts::Children childs = vtslibs::vts::children(nodeId);
    for (uint32 i = 0; i < childs.size(); i++)
        if (childsAvailable[i])
            traverse(nodeInfo.child(childs[i]));
}

bool MapImpl::panZSurfaceStack(HeightRequest &task)
{
    const TileId nodeId = task.nodeId;
    const TileId tileId = roundId(nodeId);
    UrlTemplate::Vars tileVars(tileId);
    for (auto &&it : mapConfig->surfaceStack)
    {
        const MetaTile *t = getMetaTile(it.surface->urlMeta(tileVars));
        switch (t->impl->state)
        {
        case ResourceImpl::State::initializing:
        case ResourceImpl::State::downloading:
        case ResourceImpl::State::downloaded:
            return false;
        case ResourceImpl::State::errorDownload:
        case ResourceImpl::State::errorLoad:
            continue;
        case ResourceImpl::State::ready:
            break;
        default:
            assert(false);
        }
        const MetaNode *n = t->get(nodeId, std::nothrow);
        if (!n || n->extents.ll == n->extents.ur
                || n->alien() != it.alien || !n->navtile())
            continue;
        task.metaUrl = it.surface->urlMeta(tileVars);
        task.navUrl = it.surface->urlNav(UrlTemplate::Vars(nodeId));
        return true;
    }
    assert(false);
}

void MapImpl::checkPanZQueue()
{
    if (renderer.panZQueue.empty())
        return;
    HeightRequest &task = *renderer.panZQueue.front();
    
    // find urls to the resources
    if (task.navUrl.empty() && !panZSurfaceStack(task))
        return;
    
    // acquire required resources
    const NavTile *nav = getNavTile(task.navUrl);
    const MetaTile *met = getMetaTile(task.metaUrl);
    if (!*nav || !*met)
        return;    
    const MetaNode *men = met->get(task.nodeId, std::nothrow);
    assert(men);
    
    // acquire height from the navtile
    uint32 idx = (uint32)round(task.pixPos(1)) * 256
            + (uint32)round(task.pixPos(0));
    double nhc = (double)(unsigned char)nav->data[idx] / 255.0;
    assert(nhc >= 0 && nhc <= 1);
    double nh = (nhc * (men->heightRange.max
                        - men->heightRange.min) + men->heightRange.min) ;
    
    // apply the height to camera
    double h = mapConfig->position.position[2];
    if (renderer.lastPanZShift)
        h += nh - *renderer.lastPanZShift;
    renderer.lastPanZShift.emplace(nh);
    mapConfig->position.position[2] = h;
    renderer.panZQueue.pop();
}

void MapImpl::updateCamera()
{
    // todo position conversions
    if (mapConfig->position.type
            != vtslibs::registry::Position::Type::objective)
        throw std::invalid_argument("unsupported position type");
    if (mapConfig->position.heightMode
            != vtslibs::registry::Position::HeightMode::fixed)
        throw std::invalid_argument("unsupported position height mode");
    
    checkPanZQueue();
    
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
        center = convertor->navToPhys(center);
        dir = convertor->navToPhys(dir);
        up = convertor->navToPhys(up);
        // points -> vectors
        dir = normalize(dir - center);
        up = normalize(up - center);
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        // find lat-lon coordinates of points moved to north and east
        vec3 n2 = convertor->navGeodesicDirect(center, 0, 100);
        vec3 e2 = convertor->navGeodesicDirect(center, 90, 100);
        // transform to physical srs
        center = convertor->navToPhys(center);
        vec3 n = convertor->navToPhys(n2);
        vec3 e = convertor->navToPhys(e2);
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
    
    double cameraHeight = mapConfig->position.position[2]; // todo nav2phys
    double zFactor = std::max(cameraHeight, dist) / 600000;
    double near = std::max(2.0, 40 * zFactor);
    zFactor = std::max(1.0, zFactor);
    double far = 600000 * 10 * zFactor;
    mat4 proj = perspectiveMatrix(mapConfig->position.verticalFov,
              (double)renderer.windowWidth / (double)renderer.windowHeight,
              near, far);
    
    renderer.viewProj = proj * view;
    renderer.perpendicularUnitVector = normalize(cross(up, dir));
    renderer.forwardUnitVector = dir;
}

const bool MapImpl::prerequisitesCheck()
{
    mapConfig = getMapConfig(mapConfigPath);
    if (!mapConfig || !*mapConfig)
        return false;
    
    bool ok = true;
    for (auto &&bl : mapConfig->boundLayers)
    {
        if (!bl.external())
            continue;
        std::string url = convertPath(bl.url, mapConfig->name);
        ExternalBoundLayer *r = getExternalBoundLayer(url);
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

void MapImpl::mapConfigLoaded()
{
    convertor = std::shared_ptr<CsConvertor>(CsConvertor::create(
                  mapConfig->referenceFrame.model.physicalSrs,
                  mapConfig->referenceFrame.model.navigationSrs,
                  mapConfig->referenceFrame.model.publicSrs,
                  *mapConfig
                  ));
    
    renderer.metaTileBinaryOrder = mapConfig->referenceFrame.metaBinaryOrder;
    
    mapConfig->boundInfos.clear();
    for (auto &&bl : mapConfig->boundLayers)
    {
        mapConfig->boundInfos[bl.id] = std::shared_ptr<BoundInfo>
                (new BoundInfo(bl));
    }
    
    mapConfig->generateSurfaceStack();
    
    // temporarily change height above terrain
    mapConfig->position.position[2] = 0;
    
    LOG(info3) << "position: " << mapConfig->position.position;
    LOG(info3) << "rotation: " << mapConfig->position.orientation;
    
    { // initialize camera panZQueue
        renderer.lastPanZShift.reset();
        {
            std::queue<std::shared_ptr<HeightRequest>> tmp;
            std::swap(tmp, renderer.panZQueue);
        }
        panAdjustZ(vec3to2(vecFromUblas<vec3>(
                               mapConfig->position.position)));
    }
}

void MapImpl::panAdjustZ(const vec2 &navPos)
{
    // todo
    //panZQueue.push(std::make_shared<HeightRequest>(navPos, this));
}

void MapImpl::renderTick(uint32 windowWidth, uint32 windowHeight)
{    
    if (!prerequisitesCheck())
        return;
    
    renderer.windowWidth = windowWidth;
    renderer.windowHeight = windowHeight;
    
    renderer.shaderColor = getShader("data/shaders/color.glsl");
    if (!*renderer.shaderColor)
        return;
    
    renderer.shader = getShader("data/shaders/a.glsl");
    if (!*renderer.shader)
        return;
    renderer.shader->bind();
    
    if (!convertor)
        mapConfigLoaded();
    
    updateCamera();
    
    NodeInfo nodeInfo(mapConfig->referenceFrame, TileId(),
                      false, *mapConfig);
    traverse(nodeInfo);
    
    statistics.frameIndex++;
}

void MapImpl::renderFinalize()
{}

BoundInfo *MapImpl::getBoundInfo(const std::string &id)
{
    auto it = mapConfig->boundInfos.find(id);
    if (it == mapConfig->boundInfos.end())
        return nullptr;
    return it->second.get();
}

} // namespace melown
