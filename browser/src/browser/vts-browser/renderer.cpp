#include "map.hpp"

namespace vts
{

namespace
{

bool testAndThrow(ResourceImpl::State state)
{
    switch (state)
    {
    case ResourceImpl::State::errorDownload:
    case ResourceImpl::State::errorLoad:
        throw std::runtime_error("MapConfig failed");
    case ResourceImpl::State::downloaded:
    case ResourceImpl::State::downloading:
    case ResourceImpl::State::finalizing:
    case ResourceImpl::State::initializing:
        return false;
    case ResourceImpl::State::ready:
        return true;
    default:
        assert(false);
    }
}

void normalizeAngle(double &a)
{
    a = modulo(a, 360);
}

} // namespace

void MapImpl::renderInitialize()
{}

void MapImpl::renderFinalize()
{}

void MapImpl::setMapConfigPath(const std::string &mapConfigPath)
{
    this->mapConfigPath = mapConfigPath;
    purgeHard();
}

void MapImpl::purgeHard()
{
    initialized = false;
    mapConfig.reset();
    { // clear panZQueue
        std::queue<std::shared_ptr<HeightRequest>> tmp;
        std::swap(tmp, navigation.panZQueue);
    }
    purgeSoft();
}

void MapImpl::purgeSoft()
{
    resetPositionAltitude();
    renderer.traverseRoot.reset();
    statistics.resetFrame();
    draws = DrawBatch();
    mapConfigView = "";
    
    if (!mapConfig || !*mapConfig)
        return;
    
    mapConfig->generateSurfaceStack();
    
    NodeInfo nodeInfo(mapConfig->referenceFrame, TileId(), false, *mapConfig);
    renderer.traverseRoot = std::make_shared<TraverseNode>(nodeInfo);    
}

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
    for (auto &&it : trav->surrogate)
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

bool MapImpl::visibilityTest(const TraverseNode *trav)
{
    if (trav->nodeInfo.distanceFromRoot() < 3)
        return true;
    //if (vtslibs::vts::empty(node.geomExtents))
    {
        if (!trav->extents)
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
        vec3 fl = vecFromUblas<vec3>(trav->extents->ll);
        vec3 fu = vecFromUblas<vec3>(trav->extents->ur);
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

bool MapImpl::coarsenessTest(const TraverseNode *trav)
{
    bool applyTexelSize = trav->flags & MetaNode::Flag::applyTexelSize;
    bool applyDisplaySize = trav->flags & MetaNode::Flag::applyDisplaySize;
    
    if (!applyTexelSize && !applyDisplaySize)
        return false;
    bool result = true;
    if (applyTexelSize)
    {
        assert(trav->extents);
        vec3 fl = vecFromUblas<vec3>(trav->extents->ll);
        vec3 fu = vecFromUblas<vec3>(trav->extents->ur);
        vec3 el = vecFromUblas<vec3>
                (mapConfig->referenceFrame.division.extents.ll);
        vec3 eu = vecFromUblas<vec3>
                (mapConfig->referenceFrame.division.extents.ur);
        vec3 up = renderer.perpendicularUnitVector * trav->texelSize;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 f = lowerUpperCombine(i).cwiseProduct(fu - fl) + fl;
            vec3 c1 = f.cwiseProduct(eu - el) + el - up * 0.5;
            vec3 c2 = c1 + up;
            c1 = vec4to3(renderer.viewProj * vec3to4(c1, 1), true);
            c2 = vec4to3(renderer.viewProj * vec3to4(c2, 1), true);
            double len = length(vec3(c2 - c1)) * renderer.windowHeight;
            result = result && len < options.maxTexelToPixelScale;
        }
    }
    if (applyDisplaySize)
    {
        result = false;
    }
    return result;
}

bool MapImpl::backTileCulling(const TraverseNode *trav)
{
    return false; // todo temporarily disabled backtile culling
    if (trav->nodeInfo.distanceFromRoot() < 6
            && trav->nodeInfo.distanceFromRoot() > 2)
    {
        vec2 fl = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
        vec2 fu = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
        vec2 c = (fl + fu) * 0.5;
        vec3 an = vec2to3(c, 0);
        vec3 ap = mapConfig->convertor->convert(an, trav->nodeInfo.srs(),
                    mapConfig->referenceFrame.model.physicalSrs);
        vec3 bn = vec2to3(c, 100);
        vec3 bp = mapConfig->convertor->convert(bn, trav->nodeInfo.srs(),
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

Validity MapImpl::checkMetaNode(SurfaceInfo *surface,
                                const TileId &nodeId,
                                const MetaNode *&node)
{
    if (nodeId.lod == 0)
    {
        std::string name = surface->urlMeta(UrlTemplate::Vars(roundId(nodeId)));
        std::shared_ptr<MetaTile> t = getMetaTile(name);
        Validity val = getResourceValidity(name);
        if (val == Validity::Valid)
            node = t->get(nodeId, std::nothrow);
        return val;
    }
    
    const MetaNode *pn = nullptr;
    switch (checkMetaNode(surface, vtslibs::vts::parent(nodeId), pn))
    {
    case Validity::Invalid:
        return Validity::Invalid;
    case Validity::Indeterminate:
        return Validity::Indeterminate;
    case Validity::Valid:
        break;
    default:
        assert(false);
    }

    assert(pn);
    uint32 idx = (nodeId.x % 2) + (nodeId.y % 2) * 2;
    if (pn->flags() & (MetaNode::Flag::ulChild << idx))
    {
        std::string name = surface->urlMeta(UrlTemplate::Vars(roundId(nodeId)));
        std::shared_ptr<MetaTile> t = getMetaTile(name);
        Validity val = getResourceValidity(name);
        if (val == Validity::Valid)
            node = t->get(nodeId, std::nothrow);
        return val;
    }
    
    return Validity::Invalid;
}

void MapImpl::traverseValidNode(std::shared_ptr<TraverseNode> &trav)
{    
    // node visibility
    if (backTileCulling(trav.get()) || !visibilityTest(trav.get()))
        return;
    
    touchResources(trav);
    
    // check children
    if (!trav->childs.empty() && !coarsenessTest(trav.get()))
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
    
    // render wireboxes
    if (options.renderWireBoxes)
        convertRenderTasks(draws.wires, trav->wires);
    
    // render surrogate
    if (options.renderSurrogates)
        convertRenderTasks(draws.opaque, trav->surrogate);
    
    statistics.meshesRenderedTotal++;
    statistics.meshesRenderedPerLod[
            std::min<uint32>(trav->nodeInfo.nodeId().lod,
                             MapStatistics::MaxLods-1)]++;
}

bool MapImpl::traverseDetermineSurface(std::shared_ptr<TraverseNode> &trav)
{
    assert(!trav->surface);
    assert(!trav->empty);
    const TileId nodeId = trav->nodeInfo.nodeId();
    SurfaceStackItem *topmost = nullptr;
    const MetaNode *node = nullptr;
    bool childsAvailable[4] = {false, false, false, false};
    bool determined = true;
    
    // find topmost nonempty surface
    for (auto &&it : mapConfig->surfaceStack)
    {
        const MetaNode *n = nullptr;
        switch (checkMetaNode(it.surface.get(), nodeId, n))
        {
        case Validity::Indeterminate:
            determined = false;
            // no break here
        case Validity::Invalid:
            continue;
        case Validity::Valid:
            break;
        default:
            assert(false);
        }
        assert(n);
        if (!vtslibs::vts::empty(n->geomExtents))
            trav->geomExtents = n->geomExtents;
        if (n->extents.ll != n->extents.ur)
            trav->extents = n->extents;
        for (uint32 i = 0; i < 4; i++)
            childsAvailable[i] = childsAvailable[i]
                    || (n->childFlags() & (MetaNode::Flag::ulChild << i));
        if (topmost || n->alien() != it.alien || !n->geometry()
                || n->extents.ll == n->extents.ur)
            continue;
        topmost = &it;
        node = n;
    }
    if (!determined)
        return false;
    
    if (topmost)
    {
        assert(node);
        trav->texelSize = node->texelSize;
        trav->displaySize = node->displaySize;
        trav->flags = node->flags();
        trav->extents = node->extents;
        trav->geomExtents = node->geomExtents;
        trav->surface = topmost;

        // surrogate
        trav->surrogate.clear();
        if (trav->geomExtents && vtslibs::vts::GeomExtents::validSurrogate(
                    trav->geomExtents->surrogate))
        {
            vec2 exU = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
            vec2 exL = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
            vec3 sds = vec2to3((exU + exL) * 0.5,
                               trav->geomExtents->surrogate);
            vec3 phys = mapConfig->convertor->convert(sds, trav->nodeInfo.srs(),
                                mapConfig->referenceFrame.model.physicalSrs);
            std::shared_ptr<RenderTask> r = std::make_shared<RenderTask>();
            r->mesh = getMeshRenderable("data/cube.obj");
            r->textureColor = getTexture("data/helper.jpg");
            r->model = translationMatrix(phys)
                    * scaleMatrix(trav->nodeInfo.extents().size() * 0.03);
            trav->surrogate.push_back(r);
        }
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
    
    // node update limit
    if (statistics.currentNodeUpdates++ >= options.maxNodeUpdatesPerFrame)
        return;
    
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

void MapImpl::updateCamera()
{    
    vtslibs::registry::Position &pos = mapConfig->position;
    if (pos.type != vtslibs::registry::Position::Type::objective)
        throw std::runtime_error("unsupported position type");
    if (pos.heightMode != vtslibs::registry::Position::HeightMode::fixed)
        throw std::runtime_error("unsupported position height mode");
    
    { // update and normalize camera position
        checkPanZQueue();
        
        // apply inertia
        { // position
            vec2 curXY = vec3to2(vecFromUblas<vec3>(pos.position));
            vec2 inrXY = vec3to2(navigation.inertiaMotion);
            double curZ = pos.position[2];
            double inrZ = navigation.inertiaMotion[2];
            curXY += 0.25 * inrXY;
            inrXY *= 0.8;
            curZ += 0.05 * inrZ;
            inrZ *= 0.95;
            pos.position = vecToUblas<math::Point3>(vec2to3(curXY, curZ));
            navigation.inertiaMotion = vec2to3(inrXY, inrZ);
        }
        { // orientation
            navigation.inertiaRotation(0) += options.autoRotateSpeed;
            pos.orientation += vecToUblas<math::Point3>(0.25 * 
                        navigation.inertiaRotation);
            navigation.inertiaRotation *= 0.8;
        }
        { // vertical view extent
            pos.verticalExtent += navigation.inertiaViewExtent * 0.2;
            navigation.inertiaViewExtent *= 0.8;
        }
        
        // normalize
        switch (mapConfig->srs.get
                (mapConfig->referenceFrame.model.navigationSrs).type)
        {
        case vtslibs::registry::Srs::Type::projected:
        {
            // nothing
        } break;
        case vtslibs::registry::Srs::Type::geographic:
        {
            pos.position[0] = modulo(pos.position[0] + 180, 360) - 180;
            assert(pos.position[0] >= -180 && pos.position[0] <= 180);
            pos.position[1] = clamp(pos.position[1], -80, 80);
        } break;
        default:
            throw std::invalid_argument("not implemented navigation srs type");
        }
        normalizeAngle(pos.orientation[0]);
        normalizeAngle(pos.orientation[1]);
        normalizeAngle(pos.orientation[2]);
        pos.orientation[1] = clamp(pos.orientation[1], 270, 350);
    }
    
    // camera-space vectors
    vec3 center = vecFromUblas<vec3>(pos.position);
    vec3 rot = vecFromUblas<vec3>(pos.orientation);
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
    
    // camera distance from object position
    double dist = pos.verticalExtent
            * 0.5 / tan(degToRad(pos.verticalFov * 0.5));
    mat4 view = lookAt(center - dir * dist, center, up);
    
    // camera projection matrix
    double cameraHeight = pos.position[2]; // todo nav2phys
    double zFactor = std::max(cameraHeight - 1000, dist);
    statistics.currentNearPlane = std::max(2.0, zFactor * 0.1);
    statistics.currentFarPlane = std::max(100000.0, zFactor * 20);
    mat4 proj = perspectiveMatrix(pos.verticalFov,
              (double)renderer.windowWidth / (double)renderer.windowHeight,
              statistics.currentNearPlane, statistics.currentFarPlane);
    
    // few other variables
    renderer.viewProj = proj * view;
    renderer.perpendicularUnitVector = normalize(cross(up, dir));
    renderer.forwardUnitVector = dir;
    
    // render object position
    if (options.renderObjectPosition)
    {
        vec3 phys = mapConfig->convertor->navToPhys(
                    vecFromUblas<vec3>(pos.position));
        std::shared_ptr<RenderTask> r = std::make_shared<RenderTask>();
        r->mesh = getMeshRenderable("data/cube.obj");
        r->textureColor = getTexture("data/helper.jpg");
        r->model = translationMatrix(phys)
                * scaleMatrix(pos.verticalExtent * 0.015);
        if (r->ready())
            draws.opaque.emplace_back(r.get(), this);
    }
}

bool MapImpl::prerequisitesCheck()
{    
    if (initialized)
        return true;
    
    if (mapConfigPath.empty())
        return false;
    
    mapConfig = getMapConfig(mapConfigPath);
    if (!testAndThrow(mapConfig->impl->state))
        return false;
    
    { // load external bound layers
        bool ok = true;
        for (auto &&bl : mapConfig->boundLayers)
        {
            if (!bl.external())
                continue;
            std::string url = convertPath(bl.url, mapConfig->name);
            std::shared_ptr<ExternalBoundLayer> r = getExternalBoundLayer(url);
            if (!testAndThrow(r->impl->state))
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
    
    purgeSoft();
    
    initialized = true;
    return true;
}

void MapImpl::renderTick(uint32 windowWidth, uint32 windowHeight)
{    
    if (!prerequisitesCheck())
        return;
    
    assert(mapConfig && *mapConfig);
    assert(mapConfig->convertor);
    assert(renderer.traverseRoot);
    
    renderer.windowWidth = windowWidth;
    renderer.windowHeight = windowHeight;
    
    statistics.currentNodeUpdates = 0;
    draws.opaque.clear();
    draws.transparent.clear();
    draws.wires.clear();
    updateCamera();
    traverse(renderer.traverseRoot, false);
    traverseClearing(renderer.traverseRoot);
    
    statistics.frameIndex++;
}

} // namespace vts
