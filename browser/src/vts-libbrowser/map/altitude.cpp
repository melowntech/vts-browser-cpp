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
#include "../gpuResource.hpp"
#include "../renderTasks.hpp"

#include <optick.h>

namespace vts
{

namespace
{

double cross(const vec2 &a, const vec2 &b)
{
    return a[0] * b[1] - a[1] * b[0];
}

vec2 quadrilateralUv(const vec2 &p,
    const vec2 &a, const vec2 &b, const vec2 &c, const vec2 &d)
{
    // http://www.iquilezles.org/www/articles/ibilinear/ibilinear.htm
    vec2 e = b - a;
    if (dot(e, e) < 1e-7)
        return nan2(); // todo degenerate
    vec2 f = d - a;
    if (dot(f, f) < 1e-7)
        return nan2(); // todo degenerate
    vec2 g = a - b + c - d;
    vec2 h = p - a;
    double k2 = cross(g, f);
    if (std::abs(k2) < 1e-7)
    {
        // rectangle
        double u = h[0] / e[0];
        double v = h[1] / f[1];
        if (v >= 0.0 && v <= 1.0 && u >= 0.0 && u <= 1.0)
            return vec2(u, v);
        return nan2();
    }
    double k0 = cross(h, e);
    double k1 = cross(e, f) + cross(h, g);
    double w = k1 * k1 - 4.0 * k0 * k2;
    if (w < 0.0)
        return nan2();
    w = sqrt(w);
    double v1 = (-k1 - w) / (2.0 * k2);
    double u1 = (h[0] - f[0] * v1) / (e[0] + g[0] * v1);
    double v2 = (-k1 + w) / (2.0 * k2);
    double u2 = (h[0] - f[0] * v2) / (e[0] + g[0] * v2);
    if (v1 >= 0.0 && v1 <= 1.0 && u1 >= 0.0 && u1 <= 1.0)
        return vec2(u1, v1);
    if (v2 >= 0.0 && v2 <= 1.0 && u2 >= 0.0 && u2 <= 1.0)
        return vec2(u2, v2);
    return nan2();
}

double altitudeInterpolation(
    const vec2 &query,
    const vec2 points[4],
    double values[4])
{
    vec2 uv = quadrilateralUv(query,
        points[0], points[1], points[3], points[2]);
    if (std::isnan(uv[0]) || std::isnan(uv[1]))
        return nan1();
    assert(uv[0] >= 0 && uv[0] <= 1 && uv[1] >= 0 && uv[1] <= 1);
    double p = interpolate(values[0], values[1], uv[0]);
    double q = interpolate(values[2], values[3], uv[0]);
    return interpolate(p, q, uv[1]);
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
    const vec3 &navPos, double sampleSize,
    CameraImpl *debugCamera)
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
        // todo periodicity
    }

    // find the actual corners
    TraverseNode *travRoot = findTravById(root, info->nodeId());
    if (!travRoot || !travRoot->meta)
        return false;
    double altitudes[4];
    const TraverseNode *nodes[4];
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
        nodes[i] = t;
    }

    // interpolate
    double res = altitudeInterpolation(sds, points, altitudes);
    bool resValid = !std::isnan(res);

    // debug visualization
    if (debugCamera)
    {
        for (int i = 0; i < 4; i++)
        {
            const TraverseNode *t = nodes[i];
            RenderSimpleTask task;
            task.mesh = getMesh("internal://data/meshes/sphere.obj");
            task.mesh->priority = std::numeric_limits<float>::infinity();
            if (*task.mesh)
            {
                task.model = translationMatrix(*t->surrogatePhys)
                    * scaleMatrix(t->nodeInfo.extents().size() * 0.035);
                float c = resValid ? 1.0 : 0.0;
                task.color = vec4f(c, c, c, 1.f);
                debugCamera->draws.infographics.push_back(
                    debugCamera->convert(task));
            }
        }
    }

    // output
    if (!resValid)
        return false;
    result = res;
    return true;
}

} // namespace vts
