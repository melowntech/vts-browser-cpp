#include "include/vts-browser/map.hpp"
#include "include/vts-browser/exceptions.hpp"
#include "map.hpp"

namespace vts
{

namespace
{

inline bool testAndThrow(FetchTaskImpl::State state, const std::string &message)
{
    switch (state)
    {
    case FetchTaskImpl::State::error:
        LOGTHROW(err4, MapConfigException) << message;
    case FetchTaskImpl::State::downloaded:
    case FetchTaskImpl::State::downloading:
    case FetchTaskImpl::State::finalizing:
    case FetchTaskImpl::State::initializing:
        return false;
    case FetchTaskImpl::State::ready:
        return true;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid resource state";
    }
    throw; // shut up compiler warning
}

inline const vec3 lowerUpperCombine(uint32 i)
{
    vec3 res;
    res(0) = (i >> 0) % 2;
    res(1) = (i >> 1) % 2;
    res(2) = (i >> 2) % 2;
    return res;
}

inline const vec4 column(const mat4 &m, uint32 index)
{
    return vec4(m(index, 0), m(index, 1), m(index, 2), m(index, 3));
}

} // namespace

MapImpl::Renderer::Renderer() :
    windowWidth(0), windowHeight(0)
{}

void MapImpl::renderInitialize()
{
    LOG(info3) << "Render initialize";
}

void MapImpl::renderFinalize()
{
    LOG(info3) << "Render finalize";
}

void MapImpl::setMapConfigPath(const std::string &mapConfigPath,
                               const std::string &authPath)
{
    LOG(info3) << "Changing map config path to '" << mapConfigPath << "', "
               << (!authPath.empty() ? "using" : "without")
               << " authentication";
    this->mapConfigPath = mapConfigPath;
    this->authPath = authPath;
    purgeHard();
}

void MapImpl::purgeHard()
{
    LOG(info2) << "Hard purge";
    
    if (auth)
        auth->impl->state = FetchTaskImpl::State::finalizing;
    if (mapConfig)
        mapConfig->impl->state = FetchTaskImpl::State::finalizing;
    
    initialized = false;
    auth.reset();
    mapConfig.reset();
    renderer.credits.purge();
    navigation.lastPanZShift.reset();
    std::queue<std::shared_ptr<class HeightRequest>>()
            .swap(navigation.panZQueue);
    purgeSoft();
}

void MapImpl::purgeSoft()
{
    LOG(info2) << "Soft purge";
    
    renderer.traverseRoot.reset();
    statistics.resetFrame();
    draws = MapDraws();
    mapConfigView = "";
    
    if (!mapConfig || !*mapConfig)
        return;
    
    mapConfig->generateSurfaceStack();
    NodeInfo nodeInfo(mapConfig->referenceFrame, TileId(), false, *mapConfig);
    renderer.traverseRoot = std::make_shared<TraverseNode>(nodeInfo);    
}

const TileId MapImpl::roundId(TileId nodeId)
{
    uint32 metaTileBinaryOrder = mapConfig->referenceFrame.metaBinaryOrder;
    return TileId (nodeId.lod,
       (nodeId.x >> metaTileBinaryOrder) << metaTileBinaryOrder,
       (nodeId.y >> metaTileBinaryOrder) << metaTileBinaryOrder);
}

Validity MapImpl::reorderBoundLayers(const NodeInfo &nodeInfo,
                                     uint32 subMeshIndex,
                                     BoundParamInfo::list &boundList)
{
    // prepare all layers
    {
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
    
    // sort by depth
    std::stable_sort(boundList.begin(), boundList.end());
    
    // skip overlapping layers
    std::reverse(boundList.begin(), boundList.end());
    auto it = boundList.begin(), et = boundList.end();
    while (it != et && (!it->watertight || it->transparent))
        it++;
    if (it != et)
        boundList.erase(++it, et);
    std::reverse(boundList.begin(), boundList.end());
    
    return Validity::Valid;
}

void MapImpl::touchResources(std::shared_ptr<TraverseNode> trav)
{
    trav->lastAccessTime = statistics.frameIndex;
    for (auto &&it : trav->draws)
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
    for (uint32 i = 0; i < 6; i++)
    {
        vec4 &p = renderer.frustumPlanes[i]; // current plane
        vec3 pv = vec3( // current p-vertex
                trav->aabbPhys[!!(p[0] > 0)](0),
                trav->aabbPhys[!!(p[1] > 0)](1),
                trav->aabbPhys[!!(p[2] > 0)](2));
        double d = dot(vec4to3(p), pv);
        if (d < -p[3])
            return false;
    }
    return true;
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
        vec3 up = renderer.perpendicularUnitVector * trav->texelSize;
        for (const vec3 &c : trav->cornersPhys)
        {
            vec3 c1 = c - up * 0.5;
            vec3 c2 = c1 + up;
            c1 = vec4to3(renderer.viewProj * vec3to4(c1, 1), true);
            c2 = vec4to3(renderer.viewProj * vec3to4(c2, 1), true);
            double len = length(vec3(c2 - c1)) * renderer.windowHeight;
            result = result && len < options.maxTexelToPixelScale;
        }
    }
    
    if (applyDisplaySize)
    {
        result = false; // todo
    }
    
    return result;
}

Validity MapImpl::checkMetaNode(MapConfig::SurfaceInfo *surface,
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
        LOGTHROW(fatal, std::invalid_argument) << "Invalid resource state";
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

void MapImpl::renderNode(std::shared_ptr<TraverseNode> &trav)
{
    // meshes
    for (std::shared_ptr<RenderTask> &r : trav->draws)
    {
        assert(r->ready());
        draws.draws.emplace_back(r.get(), this);
    }
    
    // surrogate
    if (options.renderSurrogates)
    {
        RenderTask task;
        task.mesh = getMeshRenderable("data/meshes/sphere.obj");
        task.model = translationMatrix(trav->surrogatePhys)
                * scaleMatrix(trav->nodeInfo.extents().size() * 0.03);
        if (trav->surface)
            task.color = vec3to4f(trav->surface->color, task.color(3));
        if (task.ready())
            draws.draws.emplace_back(&task, this);
    }
    
    // mesh box
    if (options.renderMeshBoxes && !trav->draws.empty())
    {
        for (std::shared_ptr<RenderTask> &r : trav->draws)
        {
            if (r->transparent)
                continue;
            RenderTask task = *r;
            task.mesh = getMeshRenderable("data/meshes/aabb.obj");
            task.textureColor = nullptr;
            task.textureMask = nullptr;
            task.color = vec4f(0, 0, 1, 1);
            if (task.ready())
                draws.draws.emplace_back(&task, this);
        }
    }
    
    // tile box
    if (options.renderTileBoxes)
    {
        RenderTask task;
        task.mesh = getMeshRenderable("data/meshes/line.obj");
        task.color = vec4f(1, 0, 0, 1);
        if (task.ready())
        {
            static const uint32 cora[] = {
                0, 0, 1, 2, 4, 4, 5, 6, 0, 1, 2, 3
            };
            static const uint32 corb[] = {
                1, 2, 3, 3, 5, 6, 7, 7, 4, 5, 6, 7
            };
            for (uint32 i = 0; i < 12; i++)
            {
                vec3 a = trav->cornersPhys[cora[i]];
                vec3 b = trav->cornersPhys[corb[i]];
                task.model = lookAt(a, b);
                draws.draws.emplace_back(&task, this);
            }
        }
    }
    
    // credits
    for (auto it : trav->credits)
        renderer.credits.hit(Credits::Scope::Imagery, it,
                             trav->nodeInfo.distanceFromRoot());
    
    // statistics
    statistics.meshesRenderedTotal++;
    statistics.meshesRenderedPerLod[std::min<uint32>(
        trav->nodeInfo.nodeId().lod, MapStatistics::MaxLods-1)]++;
}

void MapImpl::traverseValidNode(std::shared_ptr<TraverseNode> &trav)
{    
    // node visibility
    if (!visibilityTest(trav.get()))
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
                renderer.traverseQueue.push(t);
            return;
        }
    }
    
    if (trav->empty)
        return;
    
    renderNode(trav);
}

bool MapImpl::traverseDetermineSurface(std::shared_ptr<TraverseNode> &trav)
{
    assert(!trav->surface);
    assert(!trav->empty);
    assert(trav->draws.empty());
    assert(trav->credits.empty());
    const TileId nodeId = trav->nodeInfo.nodeId();
    MapConfig::SurfaceStackItem *topmost = nullptr;
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
            LOGTHROW(fatal, std::invalid_argument) << "Invalid resource state";
        }
        assert(n);
        for (uint32 i = 0; i < 4; i++)
            childsAvailable[i] = childsAvailable[i]
                    || (n->childFlags() & (MetaNode::Flag::ulChild << i));
        if (topmost || n->alien() != it.alien)
            continue;
        if (n->geometry())
        {
            node = n;
            topmost = &it;
        }
        if (!node)
            node = n;
    }
    if (!determined)
        return false;
    
    if (node)
    {
        trav->texelSize = node->texelSize;
        trav->displaySize = node->displaySize;
        trav->flags = node->flags();
        trav->surrogateValue = node->geomExtents.surrogate;
        
        // corners
        if (!vtslibs::vts::empty(node->geomExtents)
                && !trav->nodeInfo.srs().empty()
                && !options.debugDisableMeta5)
        {
            vec2 fl = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
            vec2 fu = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
            vec3 el = vec2to3(fl, node->geomExtents.z.min);
            vec3 eu = vec2to3(fu, node->geomExtents.z.max);
            for (uint32 i = 0; i < 8; i++)
            {
                vec3 f = lowerUpperCombine(i).cwiseProduct(eu - el) + el;
                f = mapConfig->convertor->convert(f, trav->nodeInfo.srs(),
                            mapConfig->referenceFrame.model.physicalSrs);
                trav->cornersPhys[i] = f;
            }
        }
        else if (node->extents.ll != node->extents.ur)
        {
            vec3 fl = vecFromUblas<vec3>(node->extents.ll);
            vec3 fu = vecFromUblas<vec3>(node->extents.ur);
            vec3 el = vecFromUblas<vec3>
                    (mapConfig->referenceFrame.division.extents.ll);
            vec3 eu = vecFromUblas<vec3>
                    (mapConfig->referenceFrame.division.extents.ur);
            for (uint32 i = 0; i < 8; i++)
            {
                vec3 f = lowerUpperCombine(i).cwiseProduct(fu - fl) + fl;
                trav->cornersPhys[i] = f.cwiseProduct(eu - el) + el;
            }
        }
    }
    
    if (trav->nodeInfo.distanceFromRoot() > 2)
    { // aabb
        trav->aabbPhys[0] = trav->aabbPhys[1] = trav->cornersPhys[0];
        for (const vec3 &it : trav->cornersPhys)
        {
            trav->aabbPhys[0] = min(trav->aabbPhys[0], it);
            trav->aabbPhys[1] = max(trav->aabbPhys[1], it);
        }
    }
    
    // surface
    if (topmost)
    {
        assert(node);
        trav->surface = topmost;
        
        // credits
        for (auto it : node->credits())
            trav->credits.push_back(it);

        // surrogate
        if (vtslibs::vts::GeomExtents::validSurrogate(
                    node->geomExtents.surrogate))
        {
            vec2 exU = vecFromUblas<vec2>(trav->nodeInfo.extents().ur);
            vec2 exL = vecFromUblas<vec2>(trav->nodeInfo.extents().ll);
            vec3 sds = vec2to3((exU + exL) * 0.5,
                               node->geomExtents.surrogate);
            trav->surrogatePhys = mapConfig->convertor->convert(sds,
                                trav->nodeInfo.srs(),
                                mapConfig->referenceFrame.model.physicalSrs);
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
    case Validity::Valid:
        break;
    }
    
    bool determined = true;
    std::vector<std::shared_ptr<RenderTask>> newDraws;
    std::vector<vtslibs::registry::CreditId> newCredits;
    
    // iterate over all submeshes
    for (uint32 subMeshIndex = 0, e = meshAgg->submeshes.size();
         subMeshIndex != e; subMeshIndex++)
    {
        const MeshPart &part = meshAgg->submeshes[subMeshIndex];
        std::shared_ptr<GpuMesh> mesh = part.renderable;
        
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
            case Validity::Valid:
                break;
            }
            bool allTransparent = true;
            for (BoundParamInfo &b : bls)
            {
                { // credits
                    MapConfig::BoundInfo *l = b.bound;
                    assert(l);
                    for (auto &it : l->credits)
                    {
                        auto c = renderer.credits.find(it.first);
                        if (c)
                            newCredits.push_back(*c);
                    }
                }
                
                // draw task
                std::shared_ptr<RenderTask> task
                        = std::make_shared<RenderTask>();
                task->meshAgg = meshAgg;
                task->mesh = mesh;
                task->model = part.normToPhys;
                task->uvm = b.uvMatrix();
                task->textureColor = getTexture(b.bound->urlExtTex(b.vars));
                task->textureColor->impl->availTest = b.bound->availability;
                task->externalUv = true;
                task->transparent = b.transparent;
                allTransparent = allTransparent && b.transparent;
                task->color(3) = b.alpha ? *b.alpha : 1;
                if (!b.watertight)
                    task->textureMask = getTexture(b.bound->urlMask(b.vars));
                newDraws.push_back(task);
            }
            if (!allTransparent)
                continue;
        }
        
        // internal texture
        if (part.internalUv)
        {
            UrlTemplate::Vars vars(nodeId,
                    vtslibs::vts::local(trav->nodeInfo), subMeshIndex);
            std::shared_ptr<RenderTask> task
                    = std::make_shared<RenderTask>();
            task->meshAgg = meshAgg;
            task->mesh = mesh;
            task->model = part.normToPhys;
            task->uvm = upperLeftSubMatrix(identityMatrix()).cast<float>();
            task->textureColor = getTexture(
                        trav->surface->surface->urlIntTex(vars));
            task->externalUv = false;
            newDraws.insert(newDraws.begin(), task);
        }
    }
    
    if (determined)
    {
        trav->draws.insert(trav->draws.end(),
                           newDraws.begin(), newDraws.end());
        trav->credits.insert(trav->credits.end(),
                             newCredits.begin(), newCredits.end());
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
    if (statistics.currentNodeUpdates++ >= options.maxNodeUpdatesPerTick)
        return;
    
    // find surface
    if (!trav->surface && !trav->empty && !traverseDetermineSurface(trav))
        return;
    
    assert(!!trav->surface != trav->empty);
    
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
}

void MapImpl::traverseClearing(std::shared_ptr<TraverseNode> &trav)
{
    TileId id = trav->nodeInfo.nodeId();
    if (id.lod == 3)
    {
        if ((id.y * 8 + id.x) % 64 != statistics.frameIndex % 64)
            return;
    }
    
    if (trav->lastAccessTime + 5 < statistics.frameIndex)
    {
        trav->clear();
        return;
    }
    
    for (auto &&it : trav->childs)
        traverseClearing(it);
}

void MapImpl::updateCamera()
{
    updateNavigation();

    vec3 center, dir, up;
    positionToCamera(center, dir, up);

    vtslibs::registry::Position &pos = mapConfig->position;

    // camera view matrix
    double dist = pos.type == vtslibs::registry::Position::Type::objective
            ? positionObjectiveDistance() : 1e-5;
    vec3 cameraPosPhys = center - dir * dist;
    if (callbacks.cameraOverrideEye)
        callbacks.cameraOverrideEye((double*)&cameraPosPhys);
    if (callbacks.cameraOverrideTarget)
        callbacks.cameraOverrideTarget((double*)&center);
    if (callbacks.cameraOverrideUp)
        callbacks.cameraOverrideUp((double*)&up);
    mat4 view = lookAt(cameraPosPhys, center, up);
    if (callbacks.cameraOverrideView)
    {
        callbacks.cameraOverrideView((double*)&view);
        // update dir and up
        mat4 vi = view.inverse();
        cameraPosPhys = vec4to3(vi * vec4(0, 0, 0, 1), true);
        center = vec4to3(vi * vec4(0, 0, -1, 1), true);
        dir = vec4to3(vi * vec4(0, 0, -1, 0), false);
        up = vec4to3(vi * vec4(0, 1, 0, 0), false);
    }
    
    // camera projection matrix
    double near = std::max(2.0, dist * 0.1);
    double terrainAboveOrigin = length(mapConfig->convertor->navToPhys(
        vec2to3(vec3to2(vecFromUblas<vec3>(pos.position)), 0)));
    double cameraAboveOrigin = length(cameraPosPhys);
    double cameraToHorizon = cameraAboveOrigin > terrainAboveOrigin
            ? std::sqrt(cameraAboveOrigin * cameraAboveOrigin
                - terrainAboveOrigin * terrainAboveOrigin)
            : 0;
    double mountains = 5000;
    double mountainsBehindHorizon = std::sqrt((terrainAboveOrigin + mountains)
                                    * (terrainAboveOrigin + mountains)
                                    - terrainAboveOrigin * terrainAboveOrigin);
    double far = cameraToHorizon + mountainsBehindHorizon;
    double fov = pos.verticalFov;
    double aspect = (double)renderer.windowWidth/(double)renderer.windowHeight;
    if (callbacks.cameraOverrideFovAspectNearFar)
        callbacks.cameraOverrideFovAspectNearFar(fov, aspect, near, far);
    mat4 proj = perspectiveMatrix(fov, aspect, near, far);
    if (callbacks.cameraOverrideProj)
        callbacks.cameraOverrideProj((double*)&proj);
    
    // few other variables
    renderer.viewProjRender = proj * view;
    if (!options.debugDetachedCamera)
    {
        renderer.viewProj = renderer.viewProjRender;
        renderer.perpendicularUnitVector = normalize(cross(up, dir));
        renderer.forwardUnitVector = dir;
        { // frustum planes
            vec4 c0 = column(renderer.viewProj, 0);
            vec4 c1 = column(renderer.viewProj, 1);
            vec4 c2 = column(renderer.viewProj, 2);
            vec4 c3 = column(renderer.viewProj, 3);
            renderer.frustumPlanes[0] = c3 + c0;
            renderer.frustumPlanes[1] = c3 - c0;
            renderer.frustumPlanes[2] = c3 + c1;
            renderer.frustumPlanes[3] = c3 - c1;
            renderer.frustumPlanes[4] = c3 + c2;
            renderer.frustumPlanes[5] = c3 - c2;
        }
        renderer.cameraPosPhys = cameraPosPhys;
    }
    
    // render object position
    if (options.renderObjectPosition)
    {
        vec3 phys = mapConfig->convertor->navToPhys(
                    vecFromUblas<vec3>(pos.position));
        RenderTask r;
        r.mesh = getMeshRenderable("data/meshes/cube.obj");
        r.textureColor = getTexture("data/textures/helper.jpg");
        r.model = translationMatrix(phys)
                * scaleMatrix(pos.verticalExtent * 0.015);
        if (r.ready())
            draws.draws.emplace_back(&r, this);
    }
}

bool MapImpl::prerequisitesCheck()
{
    if (auth)
    {
        auth->checkTime();
        touchResource(auth);
    }
    
    if (mapConfig)
        touchResource(mapConfig);
    
    if (initialized)
        return true;
    
    if (mapConfigPath.empty())
        return false;
    
    if (!authPath.empty())
    {
        auth = getAuth(authPath);
        if (!testAndThrow(auth->impl->state, "Authentication failure."))
            return false;
    }
    
    mapConfig = getMapConfig(mapConfigPath);
    if (!testAndThrow(mapConfig->impl->state, "Map config failure."))
        return false;
    
    { // load external bound layers
        bool ok = true;
        for (auto &&bl : mapConfig->boundLayers)
        {
            if (!bl.external())
                continue;
            std::string url = MapConfig::convertPath(bl.url,
                                                     mapConfig->impl->name);
            std::shared_ptr<ExternalBoundLayer> r = getExternalBoundLayer(url);
            if (!testAndThrow(r->impl->state, "External bound layer failure."))
                ok = false;
            else
            {
                r->id = bl.id;
                r->url = MapConfig::convertPath(r->url, url);
                if (r->metaUrl)
                    r->metaUrl = MapConfig::convertPath(*r->metaUrl, url);
                if (r->maskUrl)
                    r->maskUrl = MapConfig::convertPath(*r->maskUrl, url);
                if (r->creditsUrl)
                    r->creditsUrl = MapConfig::convertPath(*r->creditsUrl, url);
                mapConfig->boundLayers.replace(*r);
            }
        }
        if (!ok)
            return false;
    }
    
    purgeSoft();

    renderer.credits.merge(mapConfig.get());
    mapConfig->boundInfos.clear();
    for (auto &bl : mapConfig->boundLayers)
    {
        for (auto &c : bl.credits)
            if (c.second)
                renderer.credits.merge(*c.second);
        mapConfig->boundInfos[bl.id]
                = std::make_shared<MapConfig::BoundInfo>(bl);
    }
    
    navigation.autoRotation(0) = mapConfig->browserOptions.autorotate;
    
    LOG(info3) << "Render prerequisites ready";
    initialized = true;
    return true;
}

void MapImpl::renderTick(uint32 windowWidth, uint32 windowHeight)
{    
    statistics.frameIndex++;
    
    if (!prerequisitesCheck())
        return;
    
    assert(!auth || *auth);
    assert(mapConfig && *mapConfig);
    assert(mapConfig->convertor);
    assert(renderer.traverseRoot);
    
    renderer.windowWidth = windowWidth;
    renderer.windowHeight = windowHeight;
    
    statistics.currentNodeUpdates = 0;
    draws.draws.clear();
    updateCamera();
    std::queue<std::shared_ptr<TraverseNode>>().swap(renderer.traverseQueue);
    renderer.traverseQueue.push(renderer.traverseRoot);
    while (!renderer.traverseQueue.empty())
    {
        traverse(renderer.traverseQueue.front(), false);
        renderer.traverseQueue.pop();
    }
    traverseClearing(renderer.traverseRoot);
    renderer.credits.tick(credits);
}

} // namespace vts
