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

double generalInterpolation(
        const vec2 &query,
        const vec2 points[4],
        double values[4])
{
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

TraverseNode *findTravById(TraverseNode *where, const TileId &id)
{
    assert(where);
    if (where->nodeInfo.nodeId().lod > id.lod)
        return nullptr;
    if (where->nodeInfo.nodeId() == id)
        return where;
    for (auto &it : where->childs)
    {
        auto r = findTravById(it.get(), id);
        if (r)
            return r;
    }
    return nullptr;
}

TraverseNode *findTravSds(TraverseNode *where,
        const vec2 &pointSds, uint32 maxLod)
{
    assert(where && where->meta);
    TraverseNode *t = where;
    while (t->nodeInfo.nodeId().lod < maxLod)
    {
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


bool MapImpl::getPositionAltitude(double &result,
            const vec3 &navPos, double samples)
{
    assert(convertor);
    assert(!layers.empty());

    TraverseNode *root = layers[0]->traverseRoot.get();
    if (!root || !root->meta)
        return false;

    // find surface division coordinates (and appropriate node info)
    vec2 sds;
    boost::optional<NodeInfo> info;
    for (auto &it : mapConfig->referenceFrame.division.nodes)
    {
        if (it.second.partitioning.mode
                != vtslibs::registry::PartitioningMode::bisection)
            continue;
        NodeInfo ni(mapConfig->referenceFrame, it.first, false, *mapConfig);
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
        -std::log2(samples / info->extents().size()));

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
        auto t = findTravSds(travRoot, points[i], desiredLod);
        if (!t)
            return false;
        if (!vtslibs::vts::GeomExtents::validSurrogate(
                    t->meta->geomExtents.surrogate))
            return false;
        math::Extents2 ext = t->nodeInfo.extents();
        points[i] = vecFromUblas<vec2>(ext.ll + ext.ur) * 0.5;
        altitudes[i] = t->meta->surrogateNav;
        minUsedLod = std::min(minUsedLod, (uint32)t->nodeInfo.nodeId().lod);
        if (options.debugRenderAltitudeShiftCorners)
        {
            RenderTask task;
            task.mesh = getMeshRenderable("internal://data/meshes/sphere.obj");
            task.mesh->priority = std::numeric_limits<float>::infinity();
            task.model = translationMatrix(t->meta->surrogatePhys)
                    * scaleMatrix(t->nodeInfo.extents().size() * 0.031);
            task.color = vec4f(1.f, 1.f, 1.f, 1.f);
            if (*task.mesh)
                navigation.renders.push_back(task);
        }
    }

    // interpolate
    result = generalInterpolation(sds, points, altitudes);
    return true;
}

void MapImpl::updatePositionAltitude(double fadeOutFactor)
{
    if (!options.enableCameraAltitudeChanges)
        return;

    double altitude;
    if (!getPositionAltitude(altitude, navigation.targetPoint,
            mapConfig->position.verticalExtent
            / options.navigationSamplesPerViewExtent))
        return;

    // set the altitude
    if (navigation.positionAltitudeReset)
    {
        navigation.targetPoint[2] = altitude
                + *navigation.positionAltitudeReset;
        navigation.positionAltitudeReset.reset();
    }
    else if (navigation.lastPositionAltitude)
    {
        navigation.targetPoint[2] += altitude
                - *navigation.lastPositionAltitude;
        if (fadeOutFactor == fadeOutFactor)
        {
            navigation.targetPoint[2] = interpolate(navigation.targetPoint[2],
                    altitude, std::min(1.0, fadeOutFactor)
                    * options.cameraAltitudeFadeOutFactor);
        }
    }
    else
    {
        navigation.targetPoint[2] = altitude;
    }
    navigation.lastPositionAltitude = altitude;
}

} // namespace vts
