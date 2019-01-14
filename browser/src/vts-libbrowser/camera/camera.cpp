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

#include "camera.hpp"

namespace vts
{

CurrentDraw::CurrentDraw(TraverseNode *trav, TraverseNode *orig) :
    trav(trav), orig(orig)
{}

OldDraw::OldDraw(const CurrentDraw &current) :
    trav(current.trav->id()),
    orig(current.orig->id()),
    age(0)
{}

OldDraw::OldDraw(const TileId &id) : trav(id), orig(id), age(0)
{}

CameraImpl::CameraImpl(MapImpl *map, Camera *cam) :
    map(map), camera(cam),
    windowWidth(0), windowHeight(0)
{}

void CameraImpl::clear()
{
    draws.clear();
    credits.clear();

    // reset statistics
    {
        for (uint32 i = 0; i < CameraStatistics::MaxLods; i++)
        {
            statistics.metaNodesTraversedPerLod[i] = 0;
            statistics.nodesRenderedPerLod[i] = 0;
        }
        statistics.metaNodesTraversedTotal = 0;
        statistics.nodesRenderedTotal = 0;
        statistics.currentNodeMetaUpdates = 0;
        statistics.currentNodeDrawsUpdates = 0;
        statistics.currentGridNodes = 0;
    }

    // clear unused camera map layers
    {
        auto it = layers.begin();
        while (it != layers.end())
        {
            if (!it->first.lock())
                it = layers.erase(it);
            else
                it++;
        }
    }
}

void CameraImpl::touchDraws(const RenderTask &task)
{
    if (task.mesh)
        map->touchResource(task.mesh);
    if (task.textureColor)
        map->touchResource(task.textureColor);
    if (task.textureMask)
        map->touchResource(task.textureMask);
}

void CameraImpl::touchDraws(const std::vector<RenderTask> &renders)
{
    for (auto &it : renders)
        touchDraws(it);
}

void CameraImpl::touchDraws(TraverseNode *trav)
{
    for (auto &it : trav->opaque)
        touchDraws(it);
    for (auto &it : trav->transparent)
        touchDraws(it);
    if (trav->touchResource)
        map->touchResource(trav->touchResource);
}

bool CameraImpl::visibilityTest(TraverseNode *trav)
{
    assert(trav->meta);
    // aabb test
    if (!aabbTest(trav->aabbPhys, cullingPlanes))
        return false;
    // additional obb test
    if (trav->obb)
    {
        TraverseNode::Obb &obb = *trav->obb;
        vec4 planes[6];
        vts::frustumPlanes(viewProjCulling * obb.rotInv, planes);
        if (!aabbTest(obb.points, planes))
            return false;
    }
    // all tests passed
    return true;
}

bool CameraImpl::coarsenessTest(TraverseNode *trav)
{
    assert(trav->meta);
    return coarsenessValue(trav) < options.maxTexelToPixelScale;
}

namespace
{

double distanceToDisk(const vec3 &diskNormal,
    const vec2 &diskHeights, double diskHalfAngle,
    const vec3 &point)
{
    double l = point.norm();
    vec3 n = point.normalized();
    double angle = std::acos(dot(diskNormal, n));
    double vertical = l > diskHeights[1] ? l - diskHeights[1] :
        l < diskHeights[0] ? diskHeights[0] - l : 0;
    double horizontal = std::max(angle - diskHalfAngle, 0.0) * l;
    double d = std::sqrt(vertical * vertical + horizontal * horizontal);
    assert(d == d && d >= 0);
    return d;
}

} // namespace

double CameraImpl::coarsenessValue(TraverseNode *trav)
{
    bool applyTexelSize = trav->meta->flags()
            & vtslibs::vts::MetaNode::Flag::applyTexelSize;
    bool applyDisplaySize = trav->meta->flags()
            & vtslibs::vts::MetaNode::Flag::applyDisplaySize;

    // the test fails by default
    if (!applyTexelSize && !applyDisplaySize)
        return std::numeric_limits<double>::infinity();

    double texelSize = 0;
    if (applyTexelSize)
    {
        texelSize = trav->meta->texelSize;
    }
    else if (applyDisplaySize)
    {
        vec3 s = trav->aabbPhys[1] - trav->aabbPhys[0];
        double m = std::max(s[0], std::max(s[1], s[2]));
        if (m == std::numeric_limits<double>::infinity())
            return m;
        texelSize = m / trav->meta->displaySize;
    }

    if (map->options.debugCoarsenessDisks
        && trav->diskHalfAngle == trav->diskHalfAngle)
    {
        // test the value at point at the distance from the disk
        double dist = distanceToDisk(trav->diskNormalPhys,
            trav->diskHeightsPhys, trav->diskHalfAngle,
            cameraPosPhys);
        double v = texelSize * windowHeight / dist;
        assert(v == v && v > 0);
        return v;
    }
    else
    {
        // test the value on all corners of node bounding box
        double result = 0;
        for (const vec3 &c : trav->cornersPhys)
        {
            vec3 up = perpendicularUnitVector * texelSize;
            vec3 c1 = c - up * 0.5;
            vec3 c2 = c1 + up;
            c1 = vec4to3(viewProjRender * vec3to4(c1, 1), true);
            c2 = vec4to3(viewProjRender * vec3to4(c2, 1), true);
            double len = std::abs(c2[1] - c1[1])
                * windowHeight * 0.5;
            result = std::max(result, len);
        }
        return result;
    }
}

void CameraImpl::renderNode(TraverseNode *trav, TraverseNode *orig)
{
    assert(trav && orig);
    assert(trav->meta);
    assert(trav->surface);
    assert(!trav->rendersEmpty());
    assert(trav->rendersReady());

    // statistics
    statistics.nodesRenderedTotal++;
    statistics.nodesRenderedPerLod[std::min<uint32>(
        trav->id().lod, CameraStatistics::MaxLods - 1)]++;

    bool isSubNode = trav != orig;

    // draws
    if (options.lodBlending)
        currentDraws.emplace_back(trav, orig);
    else
        renderNodeDraws(trav, orig, nan1());

    // colliders
    if (!isSubNode)
        for (const RenderTask &r : trav->colliders)
            draws.colliders.emplace_back(this, r);

    // surrogate
    if (options.debugRenderSurrogates && trav->surrogatePhys)
    {
        RenderTask task;
        task.mesh = map->getMesh("internal://data/meshes/sphere.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        task.model = translationMatrix(*trav->surrogatePhys)
                * scaleMatrix(trav->nodeInfo.extents().size() * 0.03);
        task.color = vec3to4f(trav->surface->color, task.color(3));
        if (task.ready())
            draws.infographics.emplace_back(this, task);
    }

    // mesh box
    if (options.debugRenderMeshBoxes)
    {
        for (RenderTask &r : trav->opaque)
        {
            RenderTask task;
            task.model = r.model;
            task.mesh = map->getMesh("internal://data/meshes/aabb.obj");
            task.mesh->priority = std::numeric_limits<float>::infinity();
            task.color = vec3to4f(trav->surface->color, task.color(3));
            if (task.ready())
                draws.infographics.emplace_back(this, task);
        }
    }

    // tile box
    if (options.debugRenderTileBoxes
            || (options.debugRenderSubtileBoxes && isSubNode))
    {
        RenderTask task;
        task.mesh = map->getMesh("internal://data/meshes/line.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        if (trav->layer->freeLayer)
        {
            switch (trav->layer->freeLayer->type)
            {
            case vtslibs::registry::FreeLayer::Type::meshTiles:
                task.color = vec4f(1, 0, 0, 1);
                break;
            case vtslibs::registry::FreeLayer::Type::geodataTiles:
                task.color = vec4f(0, 1, 0, 1);
                break;
            case vtslibs::registry::FreeLayer::Type::geodata:
                task.color = vec4f(0, 0, 1, 1);
                break;
            default:
                break;
            }
        }
        static const uint32 cora[] = {
            0, 0, 1, 2, 4, 4, 5, 6, 0, 1, 2, 3
        };
        static const uint32 corb[] = {
            1, 2, 3, 3, 5, 6, 7, 7, 4, 5, 6, 7
        };
        if (task.ready())
        {
            // regular tile box
            if (options.debugRenderTileBoxes && !isSubNode)
            {
                for (uint32 i = 0; i < 12; i++)
                {
                    vec3 a = trav->cornersPhys[cora[i]];
                    vec3 b = trav->cornersPhys[corb[i]];
                    task.model = lookAt(a, b);
                    draws.infographics.emplace_back(this, task);
                }
            }
            // sub tile box
            for (int i = 0; i < 3; i++)
                task.color[i] *= 0.5;
            if (options.debugRenderSubtileBoxes && isSubNode)
            {
                for (uint32 i = 0; i < 12; i++)
                {
                    vec3 a = orig->cornersPhys[cora[i]];
                    vec3 b = orig->cornersPhys[corb[i]];
                    task.model = lookAt(a, b);
                    draws.infographics.emplace_back(this, task);
                }
            }
        }
    }

    // credits
    for (auto &it : trav->credits)
        map->credits.hit(trav->layer->creditScope, it,
                             trav->nodeInfo.distanceFromRoot());
}

void CameraImpl::renderNode(TraverseNode *trav)
{
    renderNode(trav, trav);
}

namespace
{

bool findNodeCoarser(TraverseNode *&trav, TraverseNode *orig)
{
    if (!trav->parent)
        return false;
    trav = trav->parent;
    if (!trav->rendersEmpty() && trav->rendersReady())
        return true;
    else
        return findNodeCoarser(trav, orig);
}

} // namespace

void CameraImpl::renderNodeCoarser(TraverseNode *trav, TraverseNode *orig)
{
    if (findNodeCoarser(trav, orig))
        renderNode(trav, orig);
}

void CameraImpl::renderNodeCoarser(TraverseNode *trav)
{
    renderNodeCoarser(trav, trav);
}

namespace
{

void updateRangeToHalf(float &a, float &b, int which)
{
    a *= 0.5;
    b *= 0.5;
    if (which)
    {
        a += 0.5;
        b += 0.5;
    }
}

} // namespace

void CameraImpl::renderNodeDraws(TraverseNode *trav,
    TraverseNode *orig, float opacity)
{
    assert(trav && orig);
    assert(!trav->rendersEmpty());
    assert(trav->rendersReady());

    vec4f uvClip;
    if (trav == orig)
        uvClip = vec4f(-1, -1, 2, 2);
    else
    {
        assert(trav->id().lod < orig->id().lod);
        uvClip = vec4f(0, 0, 1, 1);
        TraverseNode *t = orig;
        while (t != trav)
        {
            auto id = t->id();
            float *arr = uvClip.data();
            updateRangeToHalf(arr[0], arr[2], id.x % 2);
            updateRangeToHalf(arr[1], arr[3], 1 - (id.y % 2));
            t = t->parent;
        }
    }

    if (opacity == opacity)
    {
        assert(opacity >= 0.f && opacity <= 1.f);
        // opaque becomes transparent
        for (const RenderTask &r : trav->opaque)
            draws.transparent.emplace_back(this, r, uvClip.data(), opacity);
    }
    else
    {
        // some neighboring subtiles may be merged together
        //   this will reduce gpu overhead on rasterization
        // since the merging process ultimately alters the rendering order
        //   it is only allowed on opaque draws
        if (trav != orig)
            opaqueSubtiles[trav].subtiles.emplace_back(orig, uvClip);
        else
            for (const RenderTask &r : trav->opaque)
                draws.opaque.emplace_back(this, r, uvClip.data(), nan1());
    }

    for (const RenderTask &r : trav->transparent)
        draws.transparent.emplace_back(this, r, uvClip.data(), opacity);
    for (const RenderTask &r : trav->geodata)
        draws.geodata.emplace_back(this, r, uvClip.data(), opacity);

    trav->lastRenderTime = map->renderTickIndex;
    orig->lastRenderTime = map->renderTickIndex;
}

namespace
{

float blendingOpacity(uint32 age, uint32 duration)
{
    /*
      opacity
        1|   +---+    
         |  /|   |\   
         | / |   | \  
         |/  |   |  \ 
        0+---+---+---+
         0  0.5  1  1.5 normalized age
    */

    assert(age >= 0 && age <= duration * 3 / 2);
    if (age < duration / 2)
        return float(age) / (duration / 2);
    if (age > duration)
        return 1 - float(age - duration) / (duration / 2);
    return nan1(); // full opacity is signaled by nan
}

struct OldHash
{
    size_t operator() (const OldDraw &d) const
    {
        std::hash<TileId> h;
        return (h(d.orig) << 1) ^ h(d.trav);
    }
};

} // namespace

bool operator == (const OldDraw &l, const OldDraw &r)
{
    return l.orig == r.orig && l.trav == r.trav;
}

void CameraImpl::resolveBlending(TraverseNode *root,
                        CameraMapLayer &layer)
{
    if (options.lodBlending == 0)
        return;

    // update blendDraws age and remove old
    {
        uint32 duration = options.lodBlendingDuration * 3 / 2;
        auto &old = layer.blendDraws;
        old.erase(std::remove_if(old.begin(), old.end(),
            [&](OldDraw &b) {
            return ++b.age > duration;
        }), old.end());
    }

    // apply current draws
    {
        uint32 halfDuration = options.lodBlendingDuration / 2;
        std::unordered_set<OldDraw, OldHash> currentSet(currentDraws.begin(),
            currentDraws.end());
        for (auto &b : layer.blendDraws)
        {
            auto it = currentSet.find(b);
            if (it != currentSet.end())
            {
                // prevent the draw from disappearing
                b.age = std::min(b.age, halfDuration);
                // prevent the draw from adding to blendDraws
                currentSet.erase(it);
            }
        }
        // add new currentDraws to blendDraws
        for (auto &c : currentSet)
            layer.blendDraws.emplace_back(c);
        currentDraws.clear();
    }

    // detect appearing draws that have nothing to blend with
    if (options.lodBlending >= 2)
    {
        uint32 halfDuration = options.lodBlendingDuration / 2;
        uint32 duration = options.lodBlendingDuration;
        std::unordered_set<TileId> opaqueTiles;
        for (auto &b : layer.blendDraws)
            if (b.age >= halfDuration && b.age <= duration)
                opaqueTiles.insert(b.orig);
        for (auto &b : layer.blendDraws)
        {
            if (b.age >= halfDuration)
                continue; // this draw has already fully appeared
            bool ok = false;
            TileId id = b.orig;
            while (id.lod > 0)
            {
                if (opaqueTiles.count(id))
                {
                    ok = true;
                    break;
                }
                id = vtslibs::vts::parent(id);
            }
            if (!ok)
                b.age = halfDuration; // skip the appearing phase
        }
    }

    // render blend draws
    for (auto &b : layer.blendDraws)
    {
        TraverseNode *trav = findTravById(root, b.trav);
        TraverseNode *orig = findTravById(trav, b.orig);
        if (!orig || trav->rendersEmpty())
            continue;
        renderNodeDraws(trav, orig, blendingOpacity(b.age,
                                    options.lodBlendingDuration));
    }
}

void CameraImpl::renderUpdate()
{
    clear();

    if (!map->mapconfigReady)
        return;

    updateNavigation(navigation, map->lastElapsedFrameTime);

    if (windowWidth == 0 || windowHeight == 0)
        return;

    // render variables
    viewActual = lookAt(eye, target, up);
    viewProjActual = apiProj * viewActual;
    if (!options.debugDetachedCamera)
    {
        vec3 off = normalize(target - eye) * options.cullingOffsetDistance;
        viewProjCulling = apiProj * lookAt(eye - off, target, up);
        viewProjRender = viewProjActual;
        vec3 forward = normalize(target - eye);
        perpendicularUnitVector
            = normalize(cross(cross(up, forward), forward));
        forwardUnitVector = forward;
        vts::frustumPlanes(viewProjCulling, cullingPlanes);
        cameraPosPhys = eye;
        focusPosPhys = target;
    }
    else
    {
        // render original camera
        RenderTask task;
        task.mesh = map->getMesh("internal://data/meshes/line.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        task.color = vec4f(0, 1, 0, 1);
        if (task.ready())
        {
            std::vector<vec3> corners;
            corners.reserve(8);
            mat4 m = viewProjRender.inverse();
            for (int x = 0; x < 2; x++)
                for (int y = 0; y < 2; y++)
                    for (int z = 0; z < 2; z++)
                        corners.push_back(vec4to3(m
                            * vec4(x * 2 - 1, y * 2 - 1, z * 2 - 1, 1), true));
            static const uint32 cora[] = {
                0, 0, 1, 2, 4, 4, 5, 6, 0, 1, 2, 3
            };
            static const uint32 corb[] = {
                1, 2, 3, 3, 5, 6, 7, 7, 4, 5, 6, 7
            };
            for (uint32 i = 0; i < 12; i++)
            {
                vec3 a = corners[cora[i]];
                vec3 b = corners[corb[i]];
                task.model = lookAt(a, b);
                draws.infographics.emplace_back(this, task);
            }
        }
    }

    // update draws camera
    {
        CameraDraws::Camera &c = draws.camera;
        matToRaw(viewActual, c.view);
        matToRaw(apiProj, c.proj);
        vecToRaw(eye, c.eye);
    }

    // traverse and generate draws
    for (auto &it : map->layers)
    {
        traverseRender(it->traverseRoot.get());
        // resolve blending
        resolveBlending(it->traverseRoot.get(), layers[it]);
        // resolve subtile merging
        for (auto &os : opaqueSubtiles)
            os.second.resolve(os.first, this);
        opaqueSubtiles.clear();
        // resolve grid preload
        gridPreloadProcess(it->traverseRoot.get());
    }
    sortOpaqueFrontToBack();

    // update camera credits
    map->credits.tick(credits);
}

namespace
{

void computeNearFar(double &near_, double &far_, double altitude,
    const MapCelestialBody &body, bool projected,
    vec3 cameraPos, vec3 cameraForward)
{
    (void)cameraForward;
    double major = body.majorRadius;
    double flat = major / body.minorRadius;
    cameraPos[2] *= flat;
    double ground = major + (altitude == altitude ? altitude : 0.0);
    double l = projected ? cameraPos[2] + major : length(cameraPos);
    double a = std::max(1.0, l - ground);
    //LOG(info4) << "altitude: " << altitude << ", ground: " << ground
    //           << ", camera: " << l << ", above: " << a;

    if (a > 2 * major)
    {
        near_ = a - major;
        far_ = l;
    }
    else
    {
        double f = std::pow(a / (2 * major), 1.1);
        near_ = interpolate(10.0, major, f);
        near_ = std::max(10.0, near_);
        far_ = std::sqrt(std::max(0.0, l * l - major * major)) + 0.1 * major;
    }
}

} // namespace

void CameraImpl::suggestedNearFar(double &near_, double &far_)
{
    bool projected = map->mapconfig->navigationSrsType()
        == vtslibs::registry::Srs::Type::projected;
    vec3 navPos = map->convertor->physToNav(eye);
    double altitude;
    if (!map->getPositionAltitude(altitude, navPos, 10))
        altitude = std::numeric_limits<double>::quiet_NaN();
    computeNearFar(near_, far_, altitude, map->body,
        projected, eye, target - eye);
}

void CameraImpl::sortOpaqueFrontToBack()
{
    vec3 e = rawToVec3(draws.camera.eye);
    std::sort(draws.opaque.begin(), draws.opaque.end(), [e](const DrawTask &a,
        const DrawTask &b) {
        vec3 va = rawToVec3(a.center).cast<double>() - e;
        vec3 vb = rawToVec3(b.center).cast<double>() - e;
        return dot(va, va) < dot(vb, vb);
    });
}

bool MapImpl::getMapRenderComplete()
{
    if (statistics.resourcesPreparing > 0)
        return false;
    for (auto &camera : cameras)
    {
        auto cam = camera.lock();
        if (cam)
        {
            if (cam->statistics.currentNodeMetaUpdates > 0
                || cam->statistics.currentNodeDrawsUpdates > 0)
                return false;
        }
    }
    return true;
}

} // namespace vts
