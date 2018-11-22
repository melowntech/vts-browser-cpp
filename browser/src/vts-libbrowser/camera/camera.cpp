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

CameraImpl::CameraImpl(MapImpl *map, Camera *cam) :
    map(map), camera(cam),
    fovyDegs(0), near_(0), far_(0),
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
    if (!aabbTest(trav->aabbPhys, frustumPlanes))
        return false;
    // additional obb test
    if (trav->obb)
    {
        TraverseNode::Obb &obb = *trav->obb;
        vec4 planes[6];
        vts::frustumPlanes(viewProj * obb.rotInv, planes);
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
            c1 = vec4to3(viewProj * vec3to4(c1, 1), true);
            c2 = vec4to3(viewProj * vec3to4(c2, 1), true);
            double len = std::abs(c2[1] - c1[1])
                * windowHeight * 0.5;
            result = std::max(result, len);
        }
        return result;
    }
}

void CameraImpl::renderNode(TraverseNode *trav,
        TraverseNode *orig, const vec4f &uvClip)
{
    assert(trav && orig);
    assert(trav->meta);
    assert(trav->surface);
    assert(!trav->rendersEmpty());
    assert(trav->rendersReady());

    // statistics
    statistics.nodesRenderedTotal++;
    statistics.nodesRenderedPerLod[std::min<uint32>(
        trav->nodeInfo.nodeId().lod, CameraStatistics::MaxLods - 1)]++;

    bool isSubNode = trav != orig;
    assert(isSubNode == (uvClip(0) > 0 || uvClip(1) > 0
                  || uvClip(2) < 1 || uvClip(3) < 1));

    // draws
    if (isSubNode)
    {
        // some neighboring subtiles may be merged together
        //   this will reduce gpu overhead on rasterization
        // since the merging process ultimately alters the rendering order
        //   it is only allowed on opaque draws
        trav->layer->opaqueSubtiles[trav].subtiles.emplace_back(orig, uvClip);
    }
    else
    {
        for (const RenderTask &r : trav->opaque)
            draws.opaque.emplace_back(this, r, uvClip.data());
    }
    for (const RenderTask &r : trav->transparent)
        draws.transparent.emplace_back(this, r, uvClip.data());
    for (const RenderTask &r : trav->geodata)
        draws.geodata.emplace_back(this, r, uvClip.data());
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

    trav->lastRenderTime = map->renderTickIndex;
}

void CameraImpl::renderNode(TraverseNode *trav)
{
    renderNode(trav, trav, vec4f(-1, -1, 2, 2));
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

void CameraImpl::renderNodeCoarser(TraverseNode *trav,
    TraverseNode *orig, vec4f uvClip)
{
    if (!trav->parent)
        return;

    auto id = trav->nodeInfo.nodeId();
    float *arr = uvClip.data();
    updateRangeToHalf(arr[0], arr[2], id.x % 2);
    updateRangeToHalf(arr[1], arr[3], 1 - (id.y % 2));

    if (!trav->parent->rendersEmpty() && trav->parent->rendersReady())
        renderNode(trav->parent, orig, uvClip);
    else
        renderNodeCoarser(trav->parent, orig, uvClip);
}

void CameraImpl::renderNodeCoarser(TraverseNode *trav)
{
    renderNodeCoarser(trav, trav, vec4f(0, 0, 1, 1));
}

void CameraImpl::updateCamera(double elapsedTime)
{
    if (windowWidth == 0 || windowHeight == 0)
        return;

    mat4 view = lookAt(eye, target, up);
    double aspect = (double)windowWidth
                  / (double)windowHeight;
    assert(fovyDegs > 1e-3 && fovyDegs < 180 - 1e-3);
    assert(aspect > 0);
    assert(near_ > 0);
    assert(far_ > near_);
    mat4 proj = perspectiveMatrix(fovyDegs, aspect, near_, far_);

    // few other variables
    viewProjRender = proj * view;
    viewRender = view;
    if (!options.debugDetachedCamera)
    {
        viewProj = viewProjRender;
        vec3 forward = normalize(target - eye);
        perpendicularUnitVector
            = normalize(cross(cross(up, forward), forward));
        forwardUnitVector = forward;
        vts::frustumPlanes(viewProj, frustumPlanes);
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
            mat4 m = viewProj.inverse();
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
        matToRaw(view, c.view);
        matToRaw(proj, c.proj);
        vecToRaw(eye, c.eye);
        c.near_ = near_;
        c.far_ = far_;
        c.aspect = aspect;
        c.fov = fovyDegs;
    }

    // traverse and generate draws
    for (auto &it : map->layers)
    {
        traverseRender(it->traverseRoot.get());
        for (auto &os : it->opaqueSubtiles)
            os.second.resolve(os.first, this);
        it->opaqueSubtiles.clear();
    }
    gridPreloadProcess();
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

} // namespace vts
