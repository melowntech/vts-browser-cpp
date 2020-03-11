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

double sign(double a)
{
    return a < 0 ? -1 : 1;
}

double lineSolver(const vec2 &p,
    const vec2 &a, const vec2 &b,
    double av, double bv)
{
    vec2 v1 = b - a;
    if (dot(v1, v1) < 1e-4)
        return av; // degenerated into a point
    vec2 v2 = p - a;
    double l1 = length(v1);
    double l2 = length(v2);
    double u = l2 / l1 * sign(dot(v2, v1));
    return interpolate(av, bv, u);
}

double triangleSolver(const vec2 &p,
    const vec2 &a, const vec2 &b, const vec2 &c,
    double av, double bv, double cv)
{
    vec2 v0 = b - a;
    vec2 v1 = c - a;
    vec2 v2 = p - a;
    double d00 = dot(v0, v0);
    double d01 = dot(v0, v1);
    double d11 = dot(v1, v1);
    double d20 = dot(v2, v0);
    double d21 = dot(v2, v1);
    {
        // detect degenerated cases (lines)
        if (d00 < 1e-4)
            return lineSolver(p, a, c, av, cv);
        if (d11 < 1e-4)
            return lineSolver(p, a, b, av, bv);
        vec2 v3 = c - b;
        double d33 = dot(v3, v3);
        if (d33 < 1e-4)
            return lineSolver(p, a, b, av, bv);
    }
    double invDenom = 1.0 / (d00 * d11 - d01 * d01);
    double v = (d11 * d20 - d01 * d21) * invDenom;
    double w = (d00 * d21 - d01 * d20) * invDenom;
    double u = 1 - v - w;
    return u * av + v * bv + w * cv;
}

double bilinearInterpolation(double u, double v,
    double av, double bv, double cv, double dv)
{
    assert(u >= 0 && u <= 1 && v >= 0 && v <= 1);
    double p = interpolate(av, bv, u);
    double q = interpolate(cv, dv, u);
    return interpolate(p, q, v);
}

double quadrilateralSolver(const vec2 &p,
    const vec2 &a, const vec2 &b, const vec2 &c, const vec2 &d,
    double av, double bv, double cv, double dv)
{
    const vec2 e = b - a;
    const vec2 f = c - a;
    {
        // detect degenerated cases (triangles)
        vec2 q3 = b - d;
        vec2 q4 = c - d;
        if (dot(e, e) < 1e-4)
            return triangleSolver(p, a, c, d, av, cv, dv);
        if (dot(f, f) < 1e-4)
            return triangleSolver(p, a, b, d, av, bv, dv);
        if (dot(q3, q3) < 1e-4)
            return triangleSolver(p, a, b, c, av, bv, cv);
        if (dot(q4, q4) < 1e-4)
            return triangleSolver(p, a, b, c, av, bv, cv);
    }
    vec2 g = a - b + d - c;
    vec2 h = p - a;
    double k2 = cross(g, f);
    if (std::abs(k2) < 1e-7)
    {
        // rectangle
        double u = h[0] / e[0];
        double v = h[1] / f[1];
        if (v >= 0 && v <= 1 && u >= 0 && u <= 1)
            return bilinearInterpolation(u, v, av, bv, cv, dv);
        return nan1(); // the query is outside the quadrilateral
    }
    double k0 = cross(h, e);
    double k1 = cross(e, f) + cross(h, g);
    double w = k1 * k1 - 4 * k0 * k2;
    if (w < 0)
        return nan1(); // the quadrilateral has negative area
    w = sqrt(w);
    double v1 = (-k1 - w) / (2 * k2);
    double u1 = (h[0] - f[0] * v1) / (e[0] + g[0] * v1);
    double v2 = (-k1 + w) / (2 * k2);
    double u2 = (h[0] - f[0] * v2) / (e[0] + g[0] * v2);
    if (v1 >= 0 && v1 <= 1 && u1 >= 0 && u1 <= 1)
        return bilinearInterpolation(u1, v1, av, bv, cv, dv);
    if (v2 >= 0 && v2 <= 1 && u2 >= 0 && u2 <= 1)
        return bilinearInterpolation(u2, v2, av, bv, cv, dv);
    return nan1(); // the query is outside the quadrilateral
}

double altitudeInterpolation(
    const vec2 &query,
    const vec2 points[4],
    double values[4])
{
    return quadrilateralSolver(query,
        points[0], points[1], points[2], points[3],
        values[0], values[1], values[2], values[3]
    );
}

TraverseNode *findTravSds(CameraImpl *camera, TraverseNode *where,
        const vec2 &pointSds, uint32 maxLod)
{
    assert(where && where->meta);
    if (where->id().lod >= maxLod)
        return where;

    math::Point2 ublasSds = vecToUblas<math::Point2>(pointSds);
    NodeInfo i = where->nodeInfo;
    for (const auto &ci : where->childs)
    {
        TraverseNode *c = ci.get();
        if (!c->nodeInfo.inside(ublasSds))
            continue;
        if (!camera->travInit(c))
            return where;
        return findTravSds(camera, c, pointSds, maxLod);
    }
    return where;
}

} // namespace

bool CameraImpl::getSurfaceOverEllipsoid(
    double &result, const vec3 &navPos,
    double sampleSize, bool renderDebug)
{
    assert(map->convertor);
    assert(!map->layers.empty());

    TraverseNode *root = map->layers[0]->traverseRoot.get();
    if (!root || !root->meta)
        return false;

    if (sampleSize <= 0)
        sampleSize = getSurfaceAltitudeSamples();

    // find surface division coordinates (and appropriate node info)
    vec2 sds;
    boost::optional<NodeInfo> info;
    uint32 index = 0;
    for (auto &it : map->mapconfig->referenceFrame.division.nodes)
    {
        struct I {
            uint32 &i; I(uint32 &i) : i(i) {} ~I() { ++i; }
        } inc(index);
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        const NodeInfo &ni
            = map->mapconfig->referenceDivisionNodeInfos[index];
        try
        {
            sds = vec3to2(map->convertor->convert(navPos,
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
        auto t = findTravSds(this, travRoot, points[i], desiredLod);
        if (!t)
            return false;
        if (!t->meta->surrogateNav)
            return false;
        math::Extents2 ext = t->nodeInfo.extents();
        points[i] = vecFromUblas<vec2>(ext.ll + ext.ur) * 0.5;
        altitudes[i] = *t->meta->surrogateNav;
        nodes[i] = t;
    }

    // interpolate
    double res = altitudeInterpolation(sds, points, altitudes);

    // debug visualization
    if (renderDebug)
    {
        RenderInfographicsTask task;
        task.mesh = map->getMesh("internal://data/meshes/sphere.obj");
        task.mesh->priority = std::numeric_limits<float>::infinity();
        if (*task.mesh)
        {
            float c = std::isnan(res) ? 0.0 : 1.0;
            task.color = vec4f(c, c, c, 0.7f);
            double scaleSum = 0;
            for (int i = 0; i < 4; i++)
            {
                const TraverseNode *t = nodes[i];
                double scale = t->nodeInfo.extents().size() * 0.035;
                task.model = translationMatrix(*t->meta->surrogatePhys)
                        * scaleMatrix(scale);
                draws.infographics.push_back(convert(task));
                scaleSum += scale;
            }
            if (!std::isnan(res))
            {
                vec3 p = navPos;
                p[2] = res;
                p = map->convertor->convert(p, Srs::Navigation, Srs::Physical);
                task.model = translationMatrix(p) * scaleMatrix(scaleSum / 4);
                task.color = vec4f(c, c, c, 1.f);
                draws.infographics.push_back(convert(task));
            }
        }
    }

    // output
    if (std::isnan(res))
        return false;
    result = res;
    return true;
}

double CameraImpl::getSurfaceAltitudeSamples()
{
    double targetDistance = length(vec3(target - eye));
    double viewExtent = targetDistance / (apiProj(1, 1) * 0.5);
    double result = viewExtent / options.samplesForAltitudeLodSelection;
    if (std::isnan(result))
        return 10;
    return result;
}

} // namespace vts
