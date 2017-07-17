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

namespace
{

/*
double sqr(double a)
{
    return a * a;
}

double generalBilinearInterpolation(
        const vec2 &query,
        const vec2 points[4],
        double values[4])
{
    double a = -points[0][0] + points[2][0];
    double b = -points[0][0] + points[1][0];
    double c = points[0][0] - points[1][0] - points[2][0] + points[3][0];
    double d = query[0] - points[0][0];
    double e = -points[0][1] + points[2][1];
    double f = -points[0][1] + points[1][1];
    double g = points[0][1] - points[1][1] - points[2][1] + points[3][1];
    double h = query[1] - points[0][1];
    double be = b * e;
    double af = a * f;
    double dg = d * g;
    double ch = c * h;
    double ce = c * e;
    double cf = c * f;
    double ag = a * g;
    double bg = b * g;
    int s = 0;
    double alpha[2], beta[2];
    if (ce == ag || cf == bg)
    {
        alpha[0] = (query[0] - points[3][0]) / (points[0][0] - points[3][0]);
        beta[0] = (query[1] - points[3][1]) / (points[0][1] - points[3][1]);
        assert(alpha[0] >= 0 && alpha[0] <= 1);
        assert(beta[0] >= 0 && beta[0] <= 1);
    }
    else
    {
        double t = sqrt(-4 * (ce - ag) * (d*f - b*h) + sqr(be - af + dg - ch));
        alpha[0] =- ( be - af + dg - ch + t) / (2 * (ce - ag));
        beta[0]  =  ( be - af - dg + ch + t) / (2 * (cf - bg));
        alpha[1] =  (-be + af - dg + ch + t) / (2 * (ce - ag));
        beta[1]  =-((-be + af + dg - ch + t) / (2 * (cf - bg)));
        if (alpha[0] < 0 || alpha[0] > 1 || beta[0] < 0 || beta[0] > 1)
            s = 1;
    }
    assert(alpha[s] == alpha[s] && beta[s] == beta[s]);
    return interpolate(
                interpolate(values[0], values[2], alpha[s]),
                interpolate(values[1], values[3], alpha[s]),
                beta[s]);
}

double generalInterpolation(
        const vec2 &query,
        const vec2 points[4],
        double values[4])
{
    struct Comparator
    {
        bool operator() (const vec2 &a, const vec2 &b)
        {
            if (a[0] == b[0])
                return a[1] < b[1];
            return a[0] < b[1];
        }
    };
    std::set<vec2, Comparator> s;
    for (int i = 0; i < 4; i++)
        s.insert(points[i]);
    switch (s.size())
    {
    case 1:
        return values[0];
    case 2:
        for (int i = 1; i < 4; i++)
        {
            if (points[i] != points[0])
            {
                double d1 = length(vec2(points[0] - query));
                double d2 = length(vec2(points[i] - query));
                return interpolate(values[0], values[i], d1 / (d1 + d2));
            }
        }
        assert(false);
        return 0;
    case 3:
    case 4:
        return generalBilinearInterpolation(query, points, values);
    default:
        return 0;
    }
}
*/

double generalInterpolation(
        const vec2 &query,
        const vec2 points[4],
        double values[4])
{
    /*
    // nearest neighbor
    int best = 0;
    double len = length(vec2(query - points[0]));
    for (int i = 1; i < 4; i++)
    {
        double d = length(vec2(query - points[i]));
        if (d < len)
        {
            len = d;
            best = i;
        }
    }
    return values[best];
    */
    // shepards interpolation method
    double a = 0, b = 0;
    for (int i = 0; i < 4; i++)
    {
        double d = std::pow(1 / (length(vec2(query - points[i])) + 1), 2);
        a += d * values[i];
        b += d;
    }
    return a / b;
}

} // namespace

std::shared_ptr<TraverseNode> MapImpl::findTravSds(const vec2 &pointSds,
                                                   uint32 maxLod)
{
    assert(renderer.traverseRoot && renderer.traverseRoot->meta);
    std::shared_ptr<TraverseNode> t = renderer.traverseRoot;
    while (t->nodeInfo.nodeId().lod < maxLod)
    {
        auto p = t;
        for (auto &&it : t->childs)
        {
            if (!it->meta)
                continue;
            if (!it->nodeInfo.inside(vecToUblas<math::Point2>(pointSds)))
                continue;
            t = it;
            break;
        }
        if (t == p)
            break;
    }
    return t;
}

void MapImpl::updatePositionAltitudeShift()
{
    assert(convertor);

    statistics.desiredNavigationLod = 0;
    statistics.usedNavigationlod = 0;
    if (!renderer.traverseRoot || !renderer.traverseRoot->meta)
        return;

    // find surface division coordinates (and appropriate node info)
    vec2 sds;
    boost::optional<NodeInfo> info;
    for (auto &&it : mapConfig->referenceFrame.division.nodes)
    {
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        NodeInfo ni(mapConfig->referenceFrame, it.first, false, *mapConfig);
        try
        {
            sds = vec3to2(convertor->convert(navigation.targetPoint,
                mapConfig->referenceFrame.model.navigationSrs, it.second.srs));
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
        return;

    // desired navigation lod
    uint32 desiredLod = std::max(0.0,
        -std::log2(mapConfig->position.verticalExtent / info->extents().size()
        / options.navigationSamplesPerViewExtent));
    statistics.desiredNavigationLod = desiredLod;

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
    for (int i = 0; i < 4; i++)
    {
        auto t = findTravSds(points[i], desiredLod);
        if (!vtslibs::vts::GeomExtents::validSurrogate(
                    t->meta->geomExtents.surrogate))
            return;
        math::Extents2 ext = t->nodeInfo.extents();
        points[i] = vecFromUblas<vec2>(ext.ll + ext.ur) * 0.5;
        altitudes[i] = t->meta->geomExtents.surrogate;
        minUsedLod = std::min(minUsedLod, (uint32)t->nodeInfo.nodeId().lod);
        if (options.debugRenderAltitudeShiftCorners)
        {
            RenderTask task;
            task.mesh = getMeshRenderable("data/meshes/sphere.obj");
            task.mesh->priority = std::numeric_limits<float>::infinity();
            task.model = translationMatrix(t->meta->surrogatePhys)
                    * scaleMatrix(t->nodeInfo.extents().size() * 0.031);
            task.color = vec4f(1.f, 1.f, 1.f, 1.f);
            navigation.draws.push_back(std::make_shared<RenderTask>(task));
        }
    }
    statistics.usedNavigationlod = minUsedLod;

    // interpolate
    double altitude = generalInterpolation(sds, points, altitudes);
    statistics.debug = altitude;

    // set the altitude
    if (navigation.positionAltitudeResetHeight)
    {
        navigation.targetPoint[2] = altitude
                + *navigation.positionAltitudeResetHeight;
        navigation.positionAltitudeResetHeight.reset();
    }
    else if (navigation.lastPositionAltitudeShift)
    {
        navigation.targetPoint[2] += altitude
                - *navigation.lastPositionAltitudeShift;
    }
    else
    {
        navigation.targetPoint[2] = altitude;
    }
    navigation.lastPositionAltitudeShift = altitude;
}

} // namespace vts
