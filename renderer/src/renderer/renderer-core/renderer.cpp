#include "map.h"

namespace melown
{

void MapImpl::renderInitialize()
{}

void MapImpl::renderFinalize()
{}

const TileId MapImpl::roundId(TileId nodeId)
{
    return TileId (nodeId.lod,
   (nodeId.x >> renderer.metaTileBinaryOrder) << renderer.metaTileBinaryOrder,
   (nodeId.y >> renderer.metaTileBinaryOrder) << renderer.metaTileBinaryOrder);
}

Validity MapImpl::reorderBoundLayers(const NodeInfo &nodeInfo,
                                     uint32 subMeshIndex,
                                     BoundParamInfo::list &boundList)
{
    { // prepare all layers
        bool determined = true;
        auto it = boundList.begin();
        while (it != boundList.end())
        {
            switch (it->prepare(nodeInfo, this, subMeshIndex))
            {
            case Validity::Invalid:
                it = boundList.erase(it);
                break;
            case Validity::Indeterminate:
                determined = false;
                // no break here
            case Validity::Valid:
                it++;
            }
        }
        if (!determined)
            return Validity::Indeterminate;
    }
    
    // sort by depth and priority
    std::stable_sort(boundList.begin(), boundList.end());
    std::reverse(boundList.begin(), boundList.end()); // render in reverse
    
    // skip overlapping layers
    auto it = boundList.begin(), et = boundList.end();
    while (it != et && !it->watertight)
        it++;
    if (it != et)
        boundList.erase(++it, et);
    
    return Validity::Valid;
}

void MapImpl::touchResources(std::shared_ptr<TraverseNode> trav)
{
    trav->lastAccessTime = statistics.frameIndex;
    for (auto &&it : trav->opaque)
        touchResources(it);
    for (auto &&it : trav->transparent)
        touchResources(it);
    for (auto &&it : trav->wires)
        touchResources(it);
}

void MapImpl::touchResources(std::shared_ptr<RenderTask> task)
{
    if (task->meshAgg)
        touchResource(task->meshAgg);
    if (task->textureColor)
        touchResource(task->textureColor);
    if (task->textureMask)
        touchResource(task->textureMask);
}

bool MapImpl::visibilityTest(const NodeInfo &nodeInfo, const MetaNode &node)
{
    if (nodeInfo.distanceFromRoot() < 3)
        return true;
    //if (vtslibs::vts::empty(node.geomExtents))
    {
        if (node.extents.ll == node.extents.ur)
            return true;
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

bool MapImpl::backTileCulling(const NodeInfo &nodeInfo)
{
    if (nodeInfo.distanceFromRoot() < 6 && nodeInfo.distanceFromRoot() > 2)
    {
        vec2 fl = vecFromUblas<vec2>(nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(nodeInfo.extents().ur);
        vec2 c = (fl + fu) * 0.5;
        vec3 an = vec2to3(c, 0);
        vec3 ap = mapConfig->convertor->convert(an, nodeInfo.srs(),
                    mapConfig->referenceFrame.model.physicalSrs);
        vec3 bn = vec2to3(c, 100);
        vec3 bp = mapConfig->convertor->convert(bn, nodeInfo.srs(),
                    mapConfig->referenceFrame.model.physicalSrs);
        vec3 dir = normalize(bp - ap);
        if (dot(dir, renderer.forwardUnitVector) > 0)
            return true;
    }
    return false;
}

void MapImpl::convertRenderTasks(std::vector<DrawTask> &draws,
                        std::vector<std::shared_ptr<RenderTask>> &renders)
{
    for (std::shared_ptr<RenderTask> &r : renders)
        draws.emplace_back(r.get(), this);
}

void MapImpl::traverseValidNode(std::shared_ptr<TraverseNode> &trav)
{    
    // node visibility
    if (backTileCulling(trav->nodeInfo)
            || !visibilityTest(trav->nodeInfo, trav->metaNode))
        return;
    
    touchResources(trav);
    
    // check children
    if (!trav->childs.empty()
            && !coarsenessTest(trav->nodeInfo, trav->metaNode))
    {
        bool allOk = true;
        for (std::shared_ptr<TraverseNode> &t : trav->childs)
        {
            switch (t->validity)
            {
            case Validity::Indeterminate:
                traverse(t, true);
                // no break here
            case Validity::Invalid:
                allOk = false;
                break;
            case Validity::Valid:
                allOk = allOk && t->ready();
                touchResources(t);
                break;
            }
        }
        if (allOk)
        {
            for (std::shared_ptr<TraverseNode> &t : trav->childs)
                traverse(t, false);
            return;
        }
    }
    
    if (trav->empty)
        return;
    
    // render the node
    convertRenderTasks(draws.opaque, trav->opaque);
    convertRenderTasks(draws.transparent, trav->transparent);
    if (options.renderWireBoxes)
        convertRenderTasks(draws.wires, trav->wires);
    
    statistics.meshesRenderedTotal++;
    statistics.meshesRenderedPerLod[
            std::min<uint32>(trav->nodeInfo.nodeId().lod,
                             MapStatistics::MaxLods-1)]++;
}

bool MapImpl::traverseDetermineSurface(std::shared_ptr<TraverseNode> &trav)
{
    const TileId nodeId = trav->nodeInfo.nodeId();
    const TileId tileId = roundId(nodeId);
    UrlTemplate::Vars tileVars(tileId);
    const SurfaceStackItem *topmost = nullptr;
    const MetaNode *node = nullptr;
    bool childsAvailable[4] = {false, false, false, false};
    bool determined = true;
    
    // find topmost nonempty surface
    for (auto &&it : mapConfig->surfaceStack)
    {
        std::string metaName = it.surface->urlMeta(tileVars);
        std::shared_ptr<const MetaTile> t = getMetaTile(metaName);
        switch (getResourceValidity(metaName))
        {
        case Validity::Indeterminate:
            determined = false;
            // no break here
        case Validity::Invalid:
            continue;
        }
        const MetaNode *n = t->get(nodeId, std::nothrow);
        if (!n || n->extents.ll == n->extents.ur)
            continue;
        for (uint32 i = 0; i < 4; i++)
            childsAvailable[i] = childsAvailable[i]
                    || (n->childFlags() & (MetaNode::Flag::ulChild << i));
        if (topmost || n->alien() != it.alien || !n->geometry())
            continue;
        topmost = &it;
        node = n;
    }
    if (!determined)
        return false;
    
    if (topmost)
    {
        trav->metaNode = *node;
        trav->surface = topmost;
    }
    else
        trav->empty = true;
    
    // prepare children
    vtslibs::vts::Children childs = vtslibs::vts::children(nodeId);
    for (uint32 i = 0; i < 4; i++)
    {
        if (childsAvailable[i])
            trav->childs.push_back(std::make_shared<TraverseNode>(
                                       trav->nodeInfo.child(childs[i])));
    }
    
    return true;
}

bool MapImpl::traverseDetermineBoundLayers(std::shared_ptr<TraverseNode> &trav)
{
    const TileId nodeId = trav->nodeInfo.nodeId();
    
    // aggregate mesh
    std::string meshAggName = trav->surface->surface->urlMesh(
            UrlTemplate::Vars(nodeId, vtslibs::vts::local(trav->nodeInfo)));
    std::shared_ptr<MeshAggregate> meshAgg = getMeshAggregate(meshAggName);
    switch (getResourceValidity(meshAggName))
    {
    case Validity::Invalid:
        trav->validity = Validity::Invalid;
        // no break here
    case Validity::Indeterminate:
        return false;
    }
    
    bool determined = true;
    trav->opaque.clear();
    trav->transparent.clear();
    trav->wires.clear();
    
    // iterate over all submeshes
    for (uint32 subMeshIndex = 0, e = meshAgg->submeshes.size();
         subMeshIndex != e; subMeshIndex++)
    {
        const MeshPart &part = meshAgg->submeshes[subMeshIndex];
        std::shared_ptr<GpuMesh> mesh = part.renderable;
        
        { // wire box
            RenderTask task;
            task.mesh = getMeshRenderable("data/cube.obj");;
            task.model = part.normToPhys;
            task.color = trav->surface->color;
            trav->wires.push_back(std::make_shared<RenderTask>(task));
        }
        
        // internal texture
        if (part.internalUv)
        {
            UrlTemplate::Vars vars(nodeId,
                    vtslibs::vts::local(trav->nodeInfo), subMeshIndex);
            RenderTask task;
            task.meshAgg = meshAgg;
            task.mesh = mesh;
            task.model = part.normToPhys;
            task.uvm = upperLeftSubMatrix(identityMatrix()).cast<float>();
            task.textureColor = getTexture(
                        trav->surface->surface->urlIntTex(vars));
            task.external = false;
            trav->opaque.push_back(std::make_shared<RenderTask>(task));
            continue;
        }
        
        // external bound textures
        if (part.externalUv)
        {
            std::string surfaceName;
            if (trav->surface->surface->name.size() > 1)
                surfaceName = trav->surface->surface
                        ->name[part.surfaceReference - 1];
            else
                surfaceName = trav->surface->surface->name.back();
            const vtslibs::registry::View::BoundLayerParams::list &boundList
                    = mapConfig->view.surfaces[surfaceName];
            BoundParamInfo::list bls(boundList.begin(), boundList.end());
            if (part.textureLayer)
                bls.push_back(BoundParamInfo(
                        vtslibs::registry::View::BoundLayerParams(
                        mapConfig->boundLayers.get(part.textureLayer).id)));
            switch (reorderBoundLayers(trav->nodeInfo, subMeshIndex, bls))
            {
            case Validity::Indeterminate:
                determined = false;
                // no break here
            case Validity::Invalid:
                continue;
            }
            for (BoundParamInfo &b : bls)
            {
                RenderTask task;
                task.meshAgg = meshAgg;
                task.mesh = mesh;
                task.model = part.normToPhys;
                task.uvm = b.uvMatrix();
                task.textureColor = getTexture(b.bound->urlExtTex(b.vars));
                task.textureColor->impl->availTest
                        = b.bound->availability.get_ptr();
                task.external = true;
                if (!b.watertight)
                    task.textureMask = getTexture(b.bound->urlMask(b.vars));
                trav->opaque.push_back(std::make_shared<RenderTask>(task));
            }
        }
    }
    
    return determined;
}

void MapImpl::traverse(std::shared_ptr<TraverseNode> &trav, bool loadOnly)
{
    if (trav->validity == Validity::Invalid)
        return;
    
    // statistics
    statistics.metaNodesTraversedTotal++;
    statistics.metaNodesTraversedPerLod[
            std::min<uint32>(trav->nodeInfo.nodeId().lod,
                             MapStatistics::MaxLods-1)]++;
    
    // valid node
    if (trav->validity == Validity::Valid)
    {
        if (loadOnly)
            return;
        traverseValidNode(trav);
        return;
    }
    
    touchResources(trav);
    
    // find surface
    if (!trav->surface && !trav->empty && !traverseDetermineSurface(trav))
        return;
    
    assert(trav->surface || trav->empty);
    
    if (trav->empty)
    {
        trav->validity = Validity::Valid;
        return;
    }
    
    // prepare render batch
    if (!traverseDetermineBoundLayers(trav))
        return;
    
    // make node valid
    trav->validity = Validity::Valid;
    if (!loadOnly)
        traverse(trav, false);
}

void MapImpl::traverseClearing(std::shared_ptr<TraverseNode> &trav)
{
    if (trav->lastAccessTime + 100 < statistics.frameIndex)
    {
        trav->clear();
        return;
    }
    
    for (auto &&it : trav->childs)
        traverseClearing(it);
}

bool MapImpl::panZSurfaceStack(HeightRequest &task)
{
    const TileId nodeId = task.nodeId;
    const TileId tileId = roundId(nodeId);
    UrlTemplate::Vars tileVars(tileId);
    for (auto &&it : mapConfig->surfaceStack)
    {
        std::shared_ptr<const MetaTile> t = getMetaTile(
                    it.surface->urlMeta(tileVars));
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
    std::shared_ptr<const NavTile> nav = getNavTile(task.navUrl);
    std::shared_ptr<const MetaTile> met = getMetaTile(task.metaUrl);
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

void MapImpl::panAdjustZ(const vec2 &navPos)
{
    // todo
    //panZQueue.push(std::make_shared<HeightRequest>(navPos, this));
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
        center = mapConfig->convertor->navToPhys(center);
        dir = mapConfig->convertor->navToPhys(dir);
        up = mapConfig->convertor->navToPhys(up);
        // points -> vectors
        dir = normalize(dir - center);
        up = normalize(up - center);
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        // find lat-lon coordinates of points moved to north and east
        vec3 n2 = mapConfig->convertor->navGeodesicDirect(center, 0, 100);
        vec3 e2 = mapConfig->convertor->navGeodesicDirect(center, 90, 100);
        // transform to physical srs
        center = mapConfig->convertor->navToPhys(center);
        vec3 n = mapConfig->convertor->navToPhys(n2);
        vec3 e = mapConfig->convertor->navToPhys(e2);
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

bool MapImpl::prerequisitesCheck()
{
    // clear on change
    if (mapConfig && mapConfig->name != mapConfigPath)
    {
        initialized = false;
        renderer.traverseRoot.reset();
        { // clear panZQueue
            std::queue<std::shared_ptr<HeightRequest>> tmp;
            std::swap(tmp, renderer.panZQueue);
        }
    }
    
    if (initialized)
        return true;
    
    if (mapConfigPath.empty())
        return false;
    
    mapConfig = getMapConfig(mapConfigPath);
    if (!*mapConfig)
        return false;
    
    { // load external bound layers
        bool ok = true;
        for (auto &&bl : mapConfig->boundLayers)
        {
            if (!bl.external())
                continue;
            std::string url = convertPath(bl.url, mapConfig->name);
            std::shared_ptr<ExternalBoundLayer> r = getExternalBoundLayer(url);
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
        if (!ok)
            return false;
    }
    
    renderer.metaTileBinaryOrder = mapConfig->referenceFrame.metaBinaryOrder;
    
    mapConfig->boundInfos.clear();
    for (auto &&bl : mapConfig->boundLayers)
    {
        mapConfig->boundInfos[bl.id] = std::shared_ptr<BoundInfo>
                (new BoundInfo(bl));
    }
    
    renderer.lastPanZShift.reset();
    panAdjustZ(vec3to2(vecFromUblas<vec3>(
                           mapConfig->position.position)));
    
    NodeInfo nodeInfo(mapConfig->referenceFrame, TileId(), false, *mapConfig);
    renderer.traverseRoot = std::make_shared<TraverseNode>(nodeInfo);
    
    initialized = true;
    return true;
}

void MapImpl::renderTick(uint32 windowWidth, uint32 windowHeight)
{    
    if (!prerequisitesCheck())
        return;
    
    renderer.windowWidth = windowWidth;
    renderer.windowHeight = windowHeight;
    
    updateCamera();
    draws.opaque.clear();
    draws.transparent.clear();
    draws.wires.clear();
    traverse(renderer.traverseRoot, false);
    traverseClearing(renderer.traverseRoot);
    
    statistics.frameIndex++;
}

} // namespace melown
