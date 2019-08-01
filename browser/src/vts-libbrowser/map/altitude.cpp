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
#include "../navigation.hpp"
#include "../traverseNode.hpp"
#include "../coordsManip.hpp"
#include "../mapLayer.hpp"
#include "../mapConfig.hpp"
#include "../map.hpp"

#include <optick.h>

namespace vts
{

namespace
{

double generalInterpolation(
        const vec2 &query,
        const vec2 points[4],
        double values[4])
{
    // shepards interpolation method
    double a = 0, b = 0;
    for (int i = 0; i < 4; i++)
    {
        double d = 1 / (length(vec2(query - points[i])) + 1);
        d *= d;
        a += d * values[i];
        b += d;
    }
    return a / b;
}

TraverseNode *findTravSds(TraverseNode *where,
        const vec2 &pointSds, uint32 maxLod, uint32 currentTime)
{
    assert(where && where->meta);
    TraverseNode *t = where;
    while (t->id().lod < maxLod)
    {
        t->lastAccessTime = currentTime;
        auto p = t;
        for (auto &it : t->childs)
        {
            if (!it->meta)
                continue;
            if (!it->nodeInfo.inside(vecToUblas<math::Point2>(pointSds)))
                continue;
            t = it.get();
            break;
        }
        if (t == p)
            break;
    }
    return t;
}

} // namespace


bool MapImpl::getSurfaceOverEllipsoid(double &result,
            const vec3 &navPos, double sampleSize)
{
    assert(convertor);
    assert(!layers.empty());

    TraverseNode *root = layers[0]->traverseRoot.get();
    if (!root || !root->meta)
        return false;

    // find surface division coordinates (and appropriate node info)
    vec2 sds;
    boost::optional<NodeInfo> info;
    uint32 index = 0;
    for (auto &it : mapconfig->referenceFrame.division.nodes)
    {
        struct I { uint32 &i; I(uint32 &i) : i(i) {} ~I() { ++i; } } inc(index);
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        const NodeInfo &ni = mapconfig->referenceDivisionNodeInfos[index];
        try
        {
            sds = vec3to2(convertor->convert(navPos,
                Srs::Navigation, it.second));
            if (!ni.inside(vecToUblas<math::Point2>(sds)))
                continue;
            info = ni;
            break;
        }
        catch(const std::exception &)
        {
            // do nothing
        }
    }
    if (!info)
        return false;

    // desired lod
    uint32 desiredLod = std::max(0.0,
        -std::log2(sampleSize / info->extents().size()));

    // find corner positions
    vec2 points[4];
    {
        NodeInfo i = *info;
        while (i.nodeId().lod < desiredLod)
        {
            for (auto j : vtslibs::vts::children(i.nodeId()))
            {
                NodeInfo k = i.child(j);
                if (!k.inside(vecToUblas<math::Point2>(sds)))
                    continue;
                i = k;
                break;
            }
        }
        math::Extents2 ext = i.extents();
        vec2 center = vecFromUblas<vec2>(ext.ll + ext.ur) * 0.5;
        vec2 size = vecFromUblas<vec2>(ext.ur - ext.ll);
        vec2 p = sds;
        if (sds(0) < center(0))
            p(0) -= size(0);
        if (sds(1) < center(1))
            p(1) -= size(1);
        points[0] = p;
        points[1] = p + vec2(size(0), 0);
        points[2] = p + vec2(0, size(1));
        points[3] = p + size;
    }

    // find the actual corners
    double altitudes[4];
    uint32 minUsedLod = -1;
    auto travRoot = findTravById(root, info->nodeId());
    if (!travRoot || !travRoot->meta)
        return false;
    for (int i = 0; i < 4; i++)
    {
        auto t = findTravSds(travRoot, points[i], desiredLod, renderTickIndex);
        if (!t)
            return false;
        if (!t->surrogateNav)
            return false;
        math::Extents2 ext = t->nodeInfo.extents();
        points[i] = vecFromUblas<vec2>(ext.ll + ext.ur) * 0.5;
        altitudes[i] = *t->surrogateNav;
        minUsedLod = std::min(minUsedLod, (uint32)t->id().lod);
        /*
        if (options.debugRenderAltitudeShiftCorners)
        {
            RenderTask task;
            task.mesh = getMesh("internal://data/meshes/sphere.obj");
            task.mesh->priority = std::numeric_limits<float>::infinity();
            task.model = translationMatrix(*t->surrogatePhys)
                    * scaleMatrix(t->nodeInfo.extents().size() * 0.031);
            task.color = vec4f(1.f, 1.f, 1.f, 1.f);
            if (*task.mesh)
                renders.push_back(task);
        }
        */
    }

    // interpolate
    result = generalInterpolation(sds, points, altitudes);
    return true;
}

} // namespace vts
