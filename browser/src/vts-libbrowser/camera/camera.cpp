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

#include "../camera.hpp"
#include "../traverseNode.hpp"
#include "../renderTasks.hpp"
#include "../map.hpp"
#include "../gpuResource.hpp"
#include "../renderInfos.hpp"
#include "../mapLayer.hpp"
#include "../mapConfig.hpp"
#include "../credits.hpp"
#include "../coordsManip.hpp"
#include "../hashTileId.hpp"
#include "../geodata.hpp"

#include <unordered_set>
#include <optick.h>

namespace vts
{

CurrentDraw::CurrentDraw(TraverseNode *trav, TraverseNode *orig) :
    trav(trav), orig(orig)
{}

OldDraw::OldDraw(const CurrentDraw &current) :
    trav(current.trav->id),
    orig(current.orig->id)
{}

OldDraw::OldDraw(const TileId &id) : trav(id), orig(id)
{}

CameraImpl::CameraImpl(MapImpl *map, Camera *cam) :
    map(map), camera(cam),
    viewProjActual(identityMatrix4()),
    viewProjRender(identityMatrix4()),
    viewProjCulling(identityMatrix4()),
    viewActual(identityMatrix4()),
    apiProj(identityMatrix4()),
    cullingPlanes { nan4(), nan4(), nan4(), nan4(), nan4(), nan4() },
    perpendicularUnitVector(nan3()),
    forwardUnitVector(nan3()),
    cameraPosPhys(nan3()),
    focusPosPhys(nan3()),
    eye(nan3()),
    target(nan3()),
    up(nan3())
{}

void CameraImpl::clear()
{
    OPTICK_EVENT();
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

namespace
{

void touchDraws(MapImpl *map, const RenderSurfaceTask &task)
{
    if (task.mesh)
        map->touchResource(task.mesh);
    if (task.textureColor)
        map->touchResource(task.textureColor);
    if (task.textureMask)
        map->touchResource(task.textureMask);
}

template<class T>
void touchDraws(MapImpl *map, const T &renders)
{
    for (auto &it : renders)
        touchDraws(map, it);
}

} // namespace

void CameraImpl::touchDraws(TraverseNode *trav)
{
    vts::touchDraws(map, trav->opaque);
    vts::touchDraws(map, trav->transparent);
    if (trav->meshAgg)
        map->touchResource(trav->meshAgg);
    if (trav->geodataAgg)
        map->touchResource(trav->geodataAgg);
}

bool CameraImpl::visibilityTest(TraverseNode *trav)
{
    assert(trav->meta);
    // aabb test
    if (!aabbTest(trav->meta->aabbPhys, cullingPlanes))
        return false;
    // additional obb test
    if (trav->meta->obb)
    {
        const MetaNode::Obb &obb = *trav->meta->obb;
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
    return coarsenessValue(trav)
        < (trav->layer->isGeodata()
        ? options.targetPixelRatioGeodata
        : options.targetPixelRatioSurfaces);
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
    assert(!std::isnan(d) && d >= 0);
    return d;
}

} // namespace

double CameraImpl::coarsenessValue(TraverseNode *trav)
{
    assert(trav->meta);
    assert(!std::isnan(trav->meta->texelSize));

    const auto &meta = trav->meta;

    if (meta->texelSize == inf1())
        return meta->texelSize;

    if (map->options.debugCoarsenessDisks
        && !std::isnan(meta->diskHalfAngle))
    {
        // test the value at point at the distance from the disk
        double dist = distanceToDisk(meta->diskNormalPhys,
            meta->diskHeightsPhys, meta->diskHalfAngle,
            cameraPosPhys);
        double v = meta->texelSize * diskNominalDistance / dist;
        assert(!std::isnan(v) && v > 0);
        return v;
    }
    else
    {
        // test the value on all corners of node bounding box
        double result = 0;
        for (uint32 i = 0; i < 8; i++)
        {
            vec3 c = meta->cornersPhys(i);
            vec3 up = perpendicularUnitVector * meta->texelSize;
            vec3 c1 = c - up * 0.5;
            vec3 c2 = c1 + up;
            c1 = vec4to3(vec4(viewProjRender * vec3to4(c1, 1)), true);
            c2 = vec4to3(vec4(viewProjRender * vec3to4(c2, 1)), true);
            double len = std::abs(c2[1] - c1[1]);
            result = std::max(result, len);
        }
        result *= windowHeight * 0.5;
        return result;
    }
}

float CameraImpl::getTextSize(float size, const std::string &text)
{
    float x = 0;
    float sizeX = size - 1;
    float sizeX2 = round(size * 0.5);

    for (uint32 i = 0, li = text.size(); i < li; i++)
    {
        uint8 c = text[i] - 32;

        switch (c)
        {
        case 1:
        case 7:
        case 12:
        case 14:
        case 27: //:
        case 28: //;
        case 64: //'
        case 73: //i
        case 76: //l
        case 84: //t
            x += sizeX2;
            break;

        default:
            x += sizeX;
            break;
        }
    }

    return x;
}

void CameraImpl::renderText(TraverseNode *trav, float x, float y,
    const vec4f &color, float size,
    const std::string &text, bool centerText)
{
    assert(trav);
    assert(trav->meta);

    RenderInfographicsTask task;
    task.mesh = map->getMesh("internal://data/meshes/rect.obj");
    task.mesh->priority = inf1();

    task.textureColor =
        map->getTexture("internal://data/textures/debugFont2.png");
    task.textureColor->priority = inf1();

    task.model = translationMatrix(*trav->meta->surrogatePhys);
    task.color = color;

    if (centerText)
        x -= round(getTextSize(size, text) * 0.5);

    float sizeX = size - 1;
    float sizeX2 = round(size * 0.5);

    if (task.ready())
    {
        // black box
        {
            auto ctask = convert(task);
            float l = getTextSize(size, text);
            ctask.data[0] = size + 2;
            ctask.data[1] = (l + 2) / (size + 2);
            ctask.data[2] = 2.0 / windowWidth;
            ctask.data[3] = 2.0 / windowHeight;
            ctask.data2[0] = -1000;
            ctask.data2[1] = 0;
            ctask.data2[2] = x - 1;
            ctask.data2[3] = y - 1;
            ctask.type = 1;
            draws.infographics.emplace_back(ctask);
        }

        for (uint32 i = 0, li = text.size(); i < li; i++)
        {
            auto ctask = convert(task);
            uint8 c = text[i] - 32;
            uint32 charPosX = (c & 15) << 4;
            uint32 charPosY = (c >> 4) * 19;

            ctask.data[0] = size;
            ctask.data[2] = 2.0 / windowWidth;
            ctask.data[3] = 2.0 / windowHeight;

            ctask.data2[0] = charPosX;
            ctask.data2[1] = charPosY;
            ctask.data2[2] = x;
            ctask.data2[3] = y;

            switch (c)
            {
            case 1:
            case 7:
            case 12:
            case 14:
            case 27: //:
            case 28: //;
            case 64: //'
            case 73: //i
            case 76: //l
            case 84: //t
                ctask.data[1] = 0.5;// sizeX2;
                x += sizeX2;
                break;

            default:
                ctask.data[1] = 1; // sizeX;
                x += sizeX;
                break;
            }

            ctask.type = 1;
            draws.infographics.emplace_back(ctask);
        }
    }
}

void CameraImpl::renderNodeBox(TraverseNode *trav, const vec4f &color)
{
    assert(trav);
    assert(trav->meta);

    RenderInfographicsTask task;
    task.mesh = map->getMesh("internal://data/meshes/aabb.obj");
    task.mesh->priority = inf1();
    if (!task.ready())
        return;

    const auto &aabbMatrix = [](const vec3 box[2]) -> mat4
    {
        return translationMatrix((box[0] + box[1]) * 0.5)
            * scaleMatrix((box[1] - box[0]) * 0.5);
    };
    if (trav->meta->obb)
    {
        task.model = trav->meta->obb->rotInv
            * aabbMatrix(trav->meta->obb->points);
    }
    else
    {
        task.model = aabbMatrix(trav->meta->aabbPhys);
    }

    task.color = color;
    draws.infographics.emplace_back(convert(task));
}

void CameraImpl::renderNode(TraverseNode *trav, TraverseNode *orig)
{
    assert(trav && orig);
    assert(trav->meta);
    assert(trav->surface);
    assert(trav->determined);
    assert(trav->rendersReady());

    trav->lastRenderTime = map->renderTickIndex;
    orig->lastRenderTime = map->renderTickIndex;
    if (trav->rendersEmpty())
        return;

    // statistics
    statistics.nodesRenderedTotal++;
    statistics.nodesRenderedPerLod[std::min<uint32>(
        trav->id.lod, CameraStatistics::MaxLods - 1)]++;

    // credits
    for (auto &it : trav->credits)
        map->credits->hit(trav->layer->creditScope, it,
            trav->meta->nodeInfo.distanceFromRoot());

    bool isSubNode = trav != orig;

    // surfaces
    if (options.lodBlending)
        currentDraws.emplace_back(trav, orig);
    else
        renderNodeDraws(trav, orig, nan1());

    // geodata & colliders
    if (!isSubNode)
    {
        if (trav->geodataAgg)
        {
            for (const ResourceInfo &r : trav->geodataAgg->renders)
            {
                DrawGeodataTask t;
                t.geodata = std::shared_ptr<void>(
                            trav->geodataAgg, r.userData.get());
                draws.geodata.emplace_back(t);
            }
        }
        for (const RenderColliderTask &r : trav->colliders)
            draws.colliders.emplace_back(convert(r));
    }

    // surrogate
    if (options.debugRenderSurrogates && trav->meta->surrogatePhys)
    {
        RenderInfographicsTask task;
        task.mesh = map->getMesh("internal://data/meshes/sphere.obj");
        task.mesh->priority = inf1();
        if (task.ready())
        {
            task.model = translationMatrix(*trav->meta->surrogatePhys)
                * scaleMatrix(trav->meta->nodeInfo.extents().size() * 0.03);
            task.color = vec3to4(trav->surface->color, task.color(3));
            draws.infographics.emplace_back(convert(task));
        }
    }

    // mesh box
    if (options.debugRenderMeshBoxes)
    {
        RenderInfographicsTask task;
        task.mesh = map->getMesh("internal://data/meshes/aabb.obj");
        task.mesh->priority = inf1();
        if (task.ready())
        {
            for (RenderSurfaceTask &r : trav->opaque)
            {
                task.model = r.model;
                task.color = vec3to4(trav->surface->color, task.color(3));
                draws.infographics.emplace_back(convert(task));
            }
        }
    }

    // tile box
    if (!options.debugRenderTileDiagnostics
        && (options.debugRenderTileBoxes
            || (options.debugRenderSubtileBoxes && isSubNode)))
    {
        vec4f color = vec4f(1, 1, 1, 1);
        if (trav->layer->freeLayer)
        {
            switch (trav->layer->freeLayer->type)
            {
            case vtslibs::registry::FreeLayer::Type::meshTiles:
                color = vec4f(1, 0, 0, 1);
                break;
            case vtslibs::registry::FreeLayer::Type::geodataTiles:
                color = vec4f(0, 1, 0, 1);
                break;
            case vtslibs::registry::FreeLayer::Type::geodata:
                color = vec4f(0, 0, 1, 1);
                break;
            default:
                color = vec4f(1, 1, 1, 1);
                break;
            }
        }

        if (options.debugRenderTileBoxes && !isSubNode)
            renderNodeBox(trav, color);

        if (options.debugRenderSubtileBoxes && isSubNode && orig->meta)
        {
            for (int i = 0; i < 3; i++)
                color[i] *= 0.5;
            renderNodeBox(orig, color);
        }
    }

    // tile options
    if (!(options.debugRenderTileGeodataOnly && !trav->layer->isGeodata())
        && options.debugRenderTileDiagnostics && !isSubNode)
    {
        renderNodeBox(trav, vec4f(0, 0, 1, 1));

        char stmp[1024];
        auto id = trav->id;
        float size = options.debugRenderTileBigText ? 12 : 8;

        if (options.debugRenderTileLod)
        {
            sprintf(stmp, "%d", id.lod);
            renderText(trav, 0, 0, vec4f(1, 0, 0, 1), size, stmp);
        }

        if (options.debugRenderTileIndices)
        {
            sprintf(stmp, "%d %d", id.x, id.y);
            renderText(trav, 0, -(size + 2), vec4f(0, 1, 1, 1), size, stmp);
        }

        if (options.debugRenderTileTexelSize)
        {
            sprintf(stmp, "%.2f %.2f",
                trav->meta->texelSize, coarsenessValue(trav));
            renderText(trav, 0, (size + 2), vec4f(1, 0, 1, 1), size, stmp);
        }

        if (options.debugRenderTileFaces)
        {
            uint32 i = 0;
            for (RenderSurfaceTask &r : trav->opaque)
            {
                if (r.mesh.get())
                {
                    sprintf(stmp, "[%d] %d", i++, r.mesh->faces);
                    renderText(trav, 0, (size + 2) * i,
                        vec4f(1, 0, 1, 1), size, stmp);
                }
            }
            for (RenderSurfaceTask &r : trav->transparent)
            {
                if (r.mesh.get())
                {
                    sprintf(stmp, "[%d] %d", i++, r.mesh->faces);
                    renderText(trav, 0, (size + 2) * i,
                        vec4f(1, 0, 1, 1), size, stmp);
                }
            }
        }

        if (options.debugRenderTileTextureSize)
        {
            uint32 i = 0;
            for (RenderSurfaceTask &r : trav->opaque)
            {
                if (r.mesh.get() && r.textureColor.get())
                {
                    sprintf(stmp, "[%d] %dx%d", i++,
                        r.textureColor->width, r.textureColor->height);
                    renderText(trav, 0, (size + 2) * i,
                        vec4f(1, 1, 1, 1), size, stmp);
                }
            }
            for (RenderSurfaceTask &r : trav->transparent)
            {
                if (r.mesh.get() && r.textureColor.get())
                {
                    sprintf(stmp, "[%d] %dx%d", i++,
                        r.textureColor->width, r.textureColor->height);
                    renderText(trav, 0, (size + 2) * i,
                        vec4f(1, 1, 1, 1), size, stmp);
                }
            }
        }

        if (options.debugRenderTileSurface && trav->surface)
        {
            std::string stmp2;
            if (trav->surface->alien)
            {
                renderText(trav, 0, (size + 2),
                    vec4f(1, 1, 1, 1), size, "<Alien>");
            }
            const auto &names = trav->surface->name;
            for (uint32 i = 0, li = names.size(); i < li; i++)
            {
                sprintf(stmp, "[%d] %s", i, names[i].c_str());
                renderText(trav, 0,
                    (size + 2) * (i + (trav->surface->alien ? 1 : 0)),
                    vec4f(1, 1, 1, 1), size, stmp);
            }
        }

        if (options.debugRenderTileBoundLayer)
        {
            uint32 i = 0;
            for (RenderSurfaceTask &r : trav->opaque)
            {
                if (!r.boundLayerId.empty())
                {
                    sprintf(stmp, "[%d] %s", i, r.boundLayerId.c_str());
                    renderText(trav, 0, (size + 2) * (i++),
                        vec4f(1, 1, 1, 1), size, stmp);
                }
            }

            for (RenderSurfaceTask &r : trav->transparent)
            {
                if (!r.boundLayerId.empty())
                {
                    sprintf(stmp, "[%d] %s", i, r.boundLayerId.c_str());
                    renderText(trav, 0, (size + 2) * (i++),
                        vec4f(1, 1, 1, 1), size, stmp);
                }
            }
        }

        if (options.debugRenderTileCredits)
        {
            uint32 i = 0;
            for (auto &it : trav->credits)
            {
                sprintf(stmp, "[%d] %s", i,
                    map->credits->findId(it).c_str());
                renderText(trav, 0, (size + 2) * (i++),
                    vec4f(1, 1, 1, 1), size, stmp);
            }
        }
    }
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
    if (trav->determined && trav->rendersReady())
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
    TraverseNode *orig, float blendingCoverage)
{
    assert(trav && orig);
    assert(trav->meta);
    assert(trav->surface);
    assert(trav->determined);
    assert(trav->rendersReady());

    trav->lastRenderTime = map->renderTickIndex;
    orig->lastRenderTime = map->renderTickIndex;
    if (trav->rendersEmpty())
        return;

    vec4f uvClip;
    if (trav == orig)
        uvClip = vec4f(-1, -1, 2, 2);
    else
    {
        assert(trav->id.lod < orig->id.lod);
        uvClip = vec4f(0, 0, 1, 1);
        TraverseNode *t = orig;
        while (t != trav)
        {
            float *arr = uvClip.data();
            updateRangeToHalf(arr[0], arr[2], t->id.x % 2);
            updateRangeToHalf(arr[1], arr[3], 1 - (t->id.y % 2));
            t = t->parent;
        }
    }

    if (trav != orig && std::isnan(blendingCoverage))
    {
        // some neighboring subtiles may be merged together
        //   this will reduce gpu overhead on rasterization
        opaqueSubtiles[trav].subtiles.emplace_back(orig, uvClip);
    }
    else if (options.lodBlendingTransparent
        && !std::isnan(blendingCoverage))
    {
        // if lod blending is considered transparent
        //   move blending draws into transparent group
        for (const RenderSurfaceTask &r : trav->opaque)
            draws.transparent.emplace_back(convert(r, uvClip,
                blendingCoverage));
    }
    else
    {
        // if lod blending uses dithering
        //   move blending draws into opaque group
        // fully opaque draws (no blending) remain in opaque group
        for (const RenderSurfaceTask &r : trav->opaque)
            draws.opaque.emplace_back(convert(r, uvClip,
                blendingCoverage));
    }

    // transparent draws always remain in transparent group
    //   irrespective of any blending
    for (const RenderSurfaceTask &r : trav->transparent)
        draws.transparent.emplace_back(convert(r, uvClip,
            blendingCoverage));
}

namespace
{

double timeToBlendingCoverage(double age, double duration)
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

    assert(age <= duration * 3 / 2);
    if (age < duration / 2)
        return double(age) / (duration / 2);
    if (age > duration)
        return 1 - double(age - duration) / (duration / 2);
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
    OPTICK_EVENT();

    // update blendDraws age and remove old
    {
        double elapsed = map->lastElapsedFrameTime;
        double duration = options.lodBlendingDuration * 3 / 2;
        auto &old = layer.blendDraws;
        old.erase(std::remove_if(old.begin(), old.end(),
            [&](OldDraw &b) {
                b.age += elapsed;
                return b.age > duration;
        }), old.end());
    }

    // apply current draws
    {
        double halfDuration = options.lodBlendingDuration / 2;
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
        double halfDuration = options.lodBlendingDuration / 2;
        double duration = options.lodBlendingDuration;
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
        if (!orig || !trav->determined)
            continue;
        renderNodeDraws(trav, orig,
            timeToBlendingCoverage(b.age, options.lodBlendingDuration));
    }
}

void CameraImpl::renderUpdate()
{
    OPTICK_EVENT();
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
        vec3 forward = normalize(vec3(target - eye));
        vec3 off = forward * options.cullingOffsetDistance;
        viewProjCulling = apiProj * lookAt(eye - off, target, up);
        viewProjRender = viewProjActual;
        perpendicularUnitVector
            = normalize(cross(cross(up, forward), forward));
        forwardUnitVector = forward;
        vts::frustumPlanes(viewProjCulling, cullingPlanes);
        cameraPosPhys = eye;
        focusPosPhys = target;
        diskNominalDistance =  windowHeight * apiProj(1, 1) * 0.5;
    }
    else
    {
        // render original camera
        RenderInfographicsTask task;
        task.mesh = map->getMesh("internal://data/meshes/line.obj");
        task.mesh->priority = inf1();
        task.color = vec4f(0, 1, 0, 1);
        if (task.ready())
        {
            std::vector<vec3> corners;
            corners.reserve(8);
            mat4 m = viewProjRender.inverse();
            for (int x = 0; x < 2; x++)
                for (int y = 0; y < 2; y++)
                    for (int z = 0; z < 2; z++)
                        corners.push_back(vec4to3(vec4(m
                           * vec4(x * 2 - 1, y * 2 - 1, z * 2 - 1, 1)), true));
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
                draws.infographics.emplace_back(convert(task));
            }
        }
    }

    // update draws camera
    {
        CameraDraws::Camera &c = draws.camera;
        matToRaw(viewActual, c.view);
        matToRaw(apiProj, c.proj);
        vecToRaw(eye, c.eye);
        c.targetDistance = length(vec3(target - eye));
        c.viewExtent = c.targetDistance / (c.proj[5] * 0.5);

        // altitudes
        {
            vec3 navPos = map->convertor->physToNav(eye);
            c.altitudeOverEllipsoid = navPos[2];
            double tmp;
            if (getSurfaceOverEllipsoid(tmp, navPos))
                c.altitudeOverSurface = c.altitudeOverEllipsoid - tmp;
            else
                c.altitudeOverSurface = nan1();
        }
    }

    // traverse and generate draws
    for (auto &it : map->layers)
    {
        if (it->surfaceStack.surfaces.empty())
            continue;
        OPTICK_EVENT("layer");
        if (!it->freeLayerName.empty())
        {
            OPTICK_TAG("freeLayerName", it->freeLayerName.c_str());
        }
        {
            OPTICK_EVENT("traversal");
            traverseRender(it->traverseRoot.get());
        }
        resolveBlending(it->traverseRoot.get(), layers[it]);
        {
            OPTICK_EVENT("subtileMerging");
            for (auto &os : opaqueSubtiles)
                os.second.resolve(os.first, this);
            opaqueSubtiles.clear();
        }
        gridPreloadProcess(it->traverseRoot.get());
    }
    sortOpaqueFrontToBack();

    // update camera credits
    map->credits->tick(credits);
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
    double ground = major + (std::isnan(altitude) ? 0.0 : altitude);
    double l = projected ? cameraPos[2] + major : length(cameraPos);
    double a = std::max(1.0, l - ground);

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
    vec3 navPos = map->convertor->physToNav(eye);
    double altitude;
    if (!getSurfaceOverEllipsoid(altitude, navPos))
        altitude = nan1();
    bool projected = map->mapconfig->navigationSrsType()
        == vtslibs::registry::Srs::Type::projected;
    computeNearFar(near_, far_, altitude, map->body,
        projected, eye, target - eye);
}

void CameraImpl::sortOpaqueFrontToBack()
{
    OPTICK_EVENT();
    vec3 e = rawToVec3(draws.camera.eye);
    std::sort(draws.opaque.begin(), draws.opaque.end(), [e](
        const DrawSurfaceTask &a, const DrawSurfaceTask &b) {
        vec3 va = rawToVec3(a.center).cast<double>() - e;
        vec3 vb = rawToVec3(b.center).cast<double>() - e;
        return dot(va, va) < dot(vb, vb);
    });
}

} // namespace vts
