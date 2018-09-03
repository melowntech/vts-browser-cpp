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

#include "map.hpp"

namespace vts
{

MapImpl::Renderer::Renderer() :
    fixedModeDistance(0), fixedModeLod(0),
    windowWidth(0), windowHeight(0), tickIndex(0)
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
                               const std::string &authPath,
                               const std::string &sriPath)
{
    LOG(info3) << "Changing map config path to <" << mapConfigPath << ">, "
               << (!authPath.empty() ? "using" : "without")
               << " authentication and "
               << (!sriPath.empty() ?
                       std::string() + "using SRI <" + sriPath + ">"
                     : "without SRI");
    this->mapConfigPath = mapConfigPath;
    resources.authPath = authPath;
    resources.sriPath = sriPath;
    purgeMapConfig();
}

void MapImpl::purgeMapConfig()
{
    LOG(info2) << "Purge map config";

    if (resources.auth)
        resources.auth->forceRedownload();
    resources.auth.reset();
    if (mapConfig)
        mapConfig->forceRedownload();
    mapConfig.reset();

    renderer.credits.purge();
    resources.searchTasks.clear();
    resetNavigationMode();
    navigation.autoRotation = 0;
    navigation.lastPositionAltitude.reset();
    navigation.positionAltitudeReset.reset();
    convertor.reset();
    body = MapCelestialBody();
    mapconfigAvailable = false;
    purgeViewCache();
}

void MapImpl::purgeViewCache()
{
    LOG(info2) << "Purge view cache";

    if (mapConfig)
        mapConfig->consolidateView();

    layers.clear();
    statistics.resetFrame();
    draws = MapDraws();
    credits = MapCredits();
    mapConfigView = "";
    mapconfigReady = false;
}

void MapImpl::touchDraws(const RenderTask &task)
{
    if (task.mesh)
        touchResource(task.mesh);
    if (task.textureColor)
        touchResource(task.textureColor);
    if (task.textureMask)
        touchResource(task.textureMask);
}

void MapImpl::touchDraws(const std::vector<RenderTask> &renders)
{
    for (auto &it : renders)
        touchDraws(it);
}

void MapImpl::touchDraws(TraverseNode *trav)
{
    for (auto &it : trav->opaque)
        touchDraws(it);
    for (auto &it : trav->transparent)
        touchDraws(it);
    if (trav->touchResource)
        touchResource(trav->touchResource);
}

bool MapImpl::visibilityTest(TraverseNode *trav)
{
    assert(trav->meta);
    // aabb test
    if (!aabbTest(trav->aabbPhys, renderer.frustumPlanes))
        return false;
    // additional obb test
    if (trav->obb)
    {
        TraverseNode::Obb &obb = *trav->obb;
        vec4 planes[6];
        frustumPlanes(renderer.viewProj * obb.rotInv, planes);
        if (!aabbTest(obb.points, planes))
            return false;
    }
    // all tests passed
    return true;
}

bool MapImpl::coarsenessTest(TraverseNode *trav)
{
    assert(trav->meta);
    return coarsenessValue(trav) < options.maxTexelToPixelScale;
}

double MapImpl::coarsenessValue(TraverseNode *trav)
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

    // test the value on all corners of node bounding box
    double result = 0;
    vec3 up = renderer.perpendicularUnitVector * texelSize;
    for (const vec3 &c : trav->cornersPhys)
    {
        vec3 c1 = c - up * 0.5;
        vec3 c2 = c1 + up;
        c1 = vec4to3(renderer.viewProj * vec3to4(c1, 1), true);
        c2 = vec4to3(renderer.viewProj * vec3to4(c2, 1), true);
        double len = std::abs(c2[1] - c1[1]) * renderer.windowHeight * 0.5;
        result = std::max(result, len);
    }
    return result;
}

void MapImpl::renderNode(TraverseNode *trav, const vec4f &uvClip)
{
    assert(trav->meta);
    assert(trav->surface);
    assert(!trav->rendersEmpty());
    assert(trav->rendersReady());
    assert(renderer.currentTraverseMode == TraverseMode::Fixed
        || visibilityTest(trav));
    assert(trav->surface);

    // statistics
    statistics.nodesRenderedTotal++;
    statistics.nodesRenderedPerLod[std::min<uint32>(
        trav->nodeInfo.nodeId().lod, MapStatistics::MaxLods - 1)]++;

    bool isSubNode = uvClip(0) > 0 || uvClip(1) > 0
                  || uvClip(2) < 1 || uvClip(3) < 1;

    // draws
    for (const RenderTask &r : trav->opaque)
        draws.opaque.emplace_back(this, r, uvClip.data());
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
        task.mesh = getMesh("internal://data/meshes/sphere.obj");
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
            task.mesh = getMesh("internal://data/meshes/aabb.obj");
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
        task.mesh = getMesh("internal://data/meshes/line.obj");
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
            if (options.debugRenderTileBoxes)
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
            if (options.debugRenderSubtileBoxes && isSubNode)
            {
                vec3 *cp = trav->cornersPhys;
                vec3 cs1[8];
                cs1[0] = interpolate(cp[0], cp[1], uvClip[0]);
                cs1[1] = interpolate(cp[0], cp[1], uvClip[2]);
                cs1[2] = interpolate(cp[2], cp[3], uvClip[0]);
                cs1[3] = interpolate(cp[2], cp[3], uvClip[2]);
                cs1[4] = interpolate(cp[4], cp[5], uvClip[0]);
                cs1[5] = interpolate(cp[4], cp[5], uvClip[2]);
                cs1[6] = interpolate(cp[6], cp[7], uvClip[0]);
                cs1[7] = interpolate(cp[6], cp[7], uvClip[2]);
                vec3 cs2[8];
                cs2[0] = interpolate(cs1[0], cs1[2], uvClip[1]);
                cs2[1] = interpolate(cs1[1], cs1[3], uvClip[1]);
                cs2[2] = interpolate(cs1[0], cs1[2], uvClip[3]);
                cs2[3] = interpolate(cs1[1], cs1[3], uvClip[3]);
                cs2[4] = interpolate(cs1[4], cs1[6], uvClip[1]);
                cs2[5] = interpolate(cs1[5], cs1[7], uvClip[1]);
                cs2[6] = interpolate(cs1[4], cs1[6], uvClip[3]);
                cs2[7] = interpolate(cs1[5], cs1[7], uvClip[3]);
                for (uint32 i = 0; i < 12; i++)
                {
                    vec3 a = cs2[cora[i]];
                    vec3 b = cs2[corb[i]];
                    task.model = lookAt(a, b);
                    draws.infographics.emplace_back(this, task);
                }
            }
        }
    }

    // credits
    for (auto &it : trav->credits)
        renderer.credits.hit(trav->layer->creditScope, it,
                             trav->nodeInfo.distanceFromRoot());

    trav->lastRenderTime = renderer.tickIndex;
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

void MapImpl::renderNodeCoarserRecursive(TraverseNode *trav, vec4f uvClip)
{
    if (!trav->parent)
        return;

    auto id = trav->nodeInfo.nodeId();
    float *arr = uvClip.data();
    updateRangeToHalf(arr[0], arr[2], id.x % 2);
    updateRangeToHalf(arr[1], arr[3], 1 - (id.y % 2));

    if (!trav->parent->rendersEmpty() && trav->parent->rendersReady())
        renderNode(trav->parent, uvClip);
    else
        renderNodeCoarserRecursive(trav->parent, uvClip);
}

bool MapImpl::prerequisitesCheck()
{
    if (resources.auth)
        resources.auth->checkTime();

    if (mapconfigReady)
        return true;

    if (mapConfigPath.empty())
        return false;

    if (!resources.authPath.empty())
    {
        resources.auth = getAuthConfig(resources.authPath);
        if (!testAndThrow(resources.auth->state, "Authentication failure."))
            return false;
    }

    mapConfig = getMapConfig(mapConfigPath);
    if (!testAndThrow(mapConfig->state, "Map config failure."))
        return false;

    if (!convertor)
    {
        convertor = CoordManip::create(
                    *mapConfig,
                    mapConfig->browserOptions.searchSrs,
                    createOptions.customSrs1,
                    createOptions.customSrs2);
    }

    if (!mapconfigAvailable)
    {
        LOG(info3) << "Map config is available.";
        mapconfigAvailable = true;
        if (callbacks.mapconfigAvailable)
        {
            callbacks.mapconfigAvailable();
            return false;
        }
    }

    if (layers.empty())
    {
        // main surface stack
        layers.push_back(std::make_shared<MapLayer>(this));

#ifdef VTS_ENABLE_FREE_LAYERS
        // free layers
        for (const auto &it : mapConfig->view.freeLayers)
            layers.push_back(std::make_shared<MapLayer>(this, it.first,
                                                        it.second));
#endif
    }

    // check all layers
    {
        bool ok = true;
        for (auto &it : layers)
        {
            if (!it->prerequisitesCheck())
                ok = false;
        }
        if (!ok)
            return false;
    }

    renderer.credits.merge(mapConfig.get());
    initializeNavigation();
    mapConfig->initializeCelestialBody();

    LOG(info2) << "Map config is ready.";
    mapconfigReady = true;
    if (callbacks.mapconfigReady)
        callbacks.mapconfigReady(); // this may change initialized state
    return mapconfigReady;
}

void MapImpl::renderTickPrepare(double elapsedTime)
{
    if (!prerequisitesCheck())
        return;

    assert(!resources.auth || *resources.auth);
    assert(mapConfig && *mapConfig);
    assert(convertor);
    assert(!layers.empty());
    assert(layers[0]->traverseRoot);

    updateNavigation(elapsedTime);
    updateSearch();
    updateSris();
    for (auto &it : layers)
        traverseClearing(it->traverseRoot.get());

    renderer.credits.tick(credits);

    if (mapConfig->atmosphereDensityTexture)
        updateAtmosphereDensity();
}

void MapImpl::renderTickRender()
{
    draws.clear();

    if (!mapconfigReady || renderer.windowWidth == 0 || renderer.windowHeight == 0)
        return;

    updateCamera();
    for (auto &it : layers)
    {
        setCurrentTraversalMode(it->getTraverseMode());
        traverseRender(it->traverseRoot.get());
    }
    gridPreloadProcess();
    for (const RenderTask &r : navigation.renders)
        draws.infographics.emplace_back(this, r);
    sortOpaqueFrontToBack();
}

void MapImpl::renderTickColliders()
{
    draws.clear();

    if (!mapconfigReady)
        return;

    setCurrentTraversalMode(TraverseMode::Fixed);

    renderer.focusPosPhys = vec3(0, 0, 0);
    if (callbacks.collidersOverrideCenter)
        callbacks.collidersOverrideCenter(renderer.focusPosPhys.data());
    if (callbacks.collidersOverrideDistance)
        callbacks.collidersOverrideDistance(renderer.fixedModeDistance);
    if (callbacks.collidersOverrideLod)
        callbacks.collidersOverrideLod(renderer.fixedModeLod);

    renderer.cameraPosPhys = renderer.focusPosPhys;
    renderer.viewRender = translationMatrix(-renderer.focusPosPhys);
    renderer.viewProjRender = renderer.viewRender;

    // update draws camera
    {
        MapDraws::Camera &c = draws.camera;
        matToRaw(renderer.viewRender, c.view);
        matToRaw(identityMatrix4(), c.proj);
        vecToRaw(renderer.focusPosPhys, c.eye);
        c.near_ = 0;
        c.far_ = renderer.fixedModeDistance;
        c.aspect = 0;
        c.fov = 0;
        c.mapProjected = mapConfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::projected;
    }

    for (auto &it : layers)
        traverseRender(it->traverseRoot.get());
}

void MapImpl::setCurrentTraversalMode(TraverseMode mode)
{
    renderer.currentTraverseMode = mode;
    if (mode == TraverseMode::Fixed)
    {
        renderer.fixedModeDistance = options.fixedTraversalDistance;
        renderer.fixedModeLod = options.fixedTraversalLod;
    }
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

void MapImpl::updateCamera()
{
    bool projected = mapConfig->navigationSrsType()
            == vtslibs::registry::Srs::Type::projected;

    vec3 objCenter, cameraForward, cameraUp;
    positionToCamera(objCenter, cameraForward, cameraUp);

    vtslibs::registry::Position &pos = mapConfig->position;

    // camera view matrix
    double objDist = pos.type == vtslibs::registry::Position::Type::objective
            ? positionObjectiveDistance() : 1e-5;
    vec3 cameraPos = objCenter - cameraForward * objDist;
    if (callbacks.cameraOverrideEye)
        callbacks.cameraOverrideEye(cameraPos.data());
    if (callbacks.cameraOverrideTarget)
        callbacks.cameraOverrideTarget(objCenter.data());
    objDist = length(vec3(objCenter - cameraPos));
    assert(objDist > 1e-7);
    if (callbacks.cameraOverrideUp)
        callbacks.cameraOverrideUp(cameraUp.data());
    assert(length(cameraUp) > 1e-7);
    mat4 view = lookAt(cameraPos, objCenter, cameraUp);
    if (callbacks.cameraOverrideView)
    {
        mat4 viewOrig = view;
        callbacks.cameraOverrideView(view.data());
        if (viewOrig != view)
        {
            // update cameraPos, cameraForward and cameraUp
            mat4 vi = view.inverse();
            cameraPos = vec4to3(vi * vec4(0, 0, 0, 1), false);
            cameraForward = vec4to3(vi * vec4(0, 0, -1, 0), false);
            cameraUp = vec4to3(vi * vec4(0, 1, 0, 0), false);
            objDist = 0;
            objCenter = cameraPos;
        }
    }

    // camera projection matrix
    double near_ = 0;
    double far_ = 0;
    {
        double altitude;
        vec3 navPos = convertor->physToNav(cameraPos);
        if (!getPositionAltitude(altitude, navPos, 10))
            altitude = std::numeric_limits<double>::quiet_NaN();
        computeNearFar(near_, far_, altitude, body, projected,
                       cameraPos, cameraForward);
    }
    double fov = pos.verticalFov;
    double aspect = (double)renderer.windowWidth
                  / (double)renderer.windowHeight;
    if (callbacks.cameraOverrideFovAspectNearFar)
        callbacks.cameraOverrideFovAspectNearFar(fov, aspect, near_, far_);
    assert(fov > 1e-3 && fov < 180 - 1e-3);
    assert(aspect > 0);
    assert(near_ > 0);
    assert(far_ > near_);
    mat4 proj = perspectiveMatrix(fov, aspect, near_, far_);
    if (callbacks.cameraOverrideProj)
        callbacks.cameraOverrideProj(proj.data());

    // few other variables
    renderer.viewProjRender = proj * view;
    renderer.viewRender = view;
    if (!options.debugDetachedCamera)
    {
        renderer.viewProj = renderer.viewProjRender;
        renderer.perpendicularUnitVector
            = normalize(cross(cross(cameraUp, cameraForward), cameraForward));
        renderer.forwardUnitVector = cameraForward;
        frustumPlanes(renderer.viewProj, renderer.frustumPlanes);
        renderer.cameraPosPhys = cameraPos;
        renderer.focusPosPhys = objCenter;
    }
    else
    {
        // render original camera
        RenderTask task;
        task.mesh = getMesh("internal://data/meshes/line.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        task.color = vec4f(0, 1, 0, 1);
        if (task.ready())
        {
            std::vector<vec3> corners;
            corners.reserve(8);
            mat4 m = renderer.viewProj.inverse();
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

    // render object position
    if (options.debugRenderObjectPosition)
    {
        vec3 phys = convertor->navToPhys(vecFromUblas<vec3>(pos.position));
        RenderTask r;
        r.mesh = getMesh("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = getTexture("internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
                * scaleMatrix(pos.verticalExtent * 0.015);
        if (r.ready())
            draws.infographics.emplace_back(this, r);
    }

    // render target position
    if (options.debugRenderTargetPosition)
    {
        vec3 phys = convertor->navToPhys(navigation.targetPoint);
        RenderTask r;
        r.mesh = getMesh("internal://data/meshes/cube.obj");
        r.mesh->priority = std::numeric_limits<float>::infinity();
        r.textureColor = getTexture("internal://data/textures/helper.jpg");
        r.textureColor->priority = std::numeric_limits<float>::infinity();
        r.model = translationMatrix(phys)
                * scaleMatrix(navigation.targetViewExtent * 0.015);
        if (r.ready())
            draws.infographics.emplace_back(this, r);
    }

    // update draws camera
    {
        MapDraws::Camera &c = draws.camera;
        matToRaw(view, c.view);
        matToRaw(proj, c.proj);
        vecToRaw(cameraPos, c.eye);
        c.near_ = near_;
        c.far_ = far_;
        c.aspect = aspect;
        c.fov = fov;
        c.mapProjected = projected;
    }

    // atmosphere
    if (mapConfig->atmosphereDensityTexture
        && *mapConfig->atmosphereDensityTexture)
    {
        draws.atmosphereDensityTexture
            = mapConfig->atmosphereDensityTexture->info.userData;
    }
}

void MapImpl::sortOpaqueFrontToBack()
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
