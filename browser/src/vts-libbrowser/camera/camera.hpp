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

#ifndef CAMERA_HPP_sd456g
#define CAMERA_HPP_sd456g

#include "../map.hpp"

#include "../include/vts-browser/camera.hpp"
#include "../include/vts-browser/cameraCredits.hpp"
#include "../include/vts-browser/cameraDraws.hpp"
#include "../include/vts-browser/cameraOptions.hpp"
#include "../include/vts-browser/cameraStatistics.hpp"

namespace std
{

template<>
struct hash<vts::TileId>
{
    size_t operator()(const vts::TileId &x) const
    {
        size_t r = std::hash<vtslibs::storage::Lod>()(x.lod) << 1;
        r ^= std::hash<vts::TileId::index_type>()(x.x) << 1;
        r ^= std::hash<vts::TileId::index_type>()(x.y);
        return r;
    }
};

} // namespace std

namespace vts
{

class CurrentDraw
{
public:
    vec4f uvClip;
    TraverseNode *trav;
    TraverseNode *orig;
    float age;

    CurrentDraw(TraverseNode *trav, TraverseNode *orig, const vec4f &uvClip);
};

class OldDraw
{
public:
    vec4f uvClip;
    TileId trav;
    TileId orig;
    float age;

    OldDraw(const CurrentDraw &current);
};

class CameraMapLayer
{
public:
    std::vector<OldDraw> blendDraws; // draws rendered in previous frames suitable for blending
    std::unordered_map<TileId, OldDraw> lastDraws; // draws requested to render in last frame
};

class SubtilesMerger
{
public:
    struct Subtile
    {
        TraverseNode *orig;
        vec4f uvClip;
        Subtile(TraverseNode *orig, const vec4f &uvClip);
    };
    std::vector<Subtile> subtiles;
    void resolve(TraverseNode *trav, CameraImpl *impl);
};

class CameraImpl
{
public:
    MapImpl *const map;
    Camera *const camera;
    std::weak_ptr<NavigationImpl> navigation;
    CameraCredits credits;
    CameraDraws draws;
    CameraOptions options;
    CameraStatistics statistics;
    std::vector<TileId> gridLoadRequests;
    std::vector<CurrentDraw> currentDraws;
    std::unordered_map<TraverseNode*, SubtilesMerger> opaqueSubtiles;
    std::map<std::weak_ptr<MapLayer>, CameraMapLayer,
            std::owner_less<std::weak_ptr<MapLayer>>> layers;
    mat4 viewProj;
    mat4 viewProjRender;
    mat4 viewRender;
    mat4 apiProj;
    vec4 frustumPlanes[6];
    vec3 perpendicularUnitVector;
    vec3 forwardUnitVector;
    vec3 cameraPosPhys;
    vec3 focusPosPhys;
    vec3 eye, target, up;
    uint32 windowWidth;
    uint32 windowHeight;

    CameraImpl(MapImpl *map, Camera *cam);
    void clear();
    Validity reorderBoundLayers(const NodeInfo &nodeInfo, uint32 subMeshIndex,
                           BoundParamInfo::List &boundList, double priority);
    void touchDraws(const RenderTask &task);
    void touchDraws(const std::vector<RenderTask> &renders);
    void touchDraws(TraverseNode *trav);
    bool visibilityTest(TraverseNode *trav);
    bool coarsenessTest(TraverseNode *trav);
    double coarsenessValue(TraverseNode *trav);
    void renderNode(TraverseNode *trav,
                    TraverseNode *orig, const vec4f &uvClip);
    void renderNode(TraverseNode *trav);
    bool findNodeCoarser(TraverseNode *&trav,
                    TraverseNode *orig, vec4f &uvClip);
    void renderNodeCoarser(TraverseNode *trav,
                    TraverseNode *orig, vec4f uvClip);
    void renderNodeCoarser(TraverseNode *trav);
    void renderNodeDraws(TraverseNode *trav,
        TraverseNode *orig, const vec4f &uvClip, float opacity);
    bool generateMonolithicGeodataTrav(TraverseNode *trav);
    std::shared_ptr<GpuTexture> travInternalTexture(TraverseNode *trav,
                                                  uint32 subMeshIndex);
    bool travDetermineMeta(TraverseNode *trav);
    void travDetermineMetaImpl(TraverseNode *trav);
    bool travDetermineDraws(TraverseNode *trav);
    bool travDetermineDrawsSurface(TraverseNode *trav);
    bool travDetermineDrawsGeodata(TraverseNode *trav);
    double travDistance(TraverseNode *trav, const vec3 pointPhys);
    void updateNodePriority(TraverseNode *trav);
    bool travInit(TraverseNode *trav);
    void travModeHierarchical(TraverseNode *trav, bool loadOnly);
    void travModeFlat(TraverseNode *trav);
    bool travModeBalanced(TraverseNode *trav, bool renderOnly);
    void travModeFixed(TraverseNode *trav);
    void traverseRender(TraverseNode *trav);
    void gridPreloadRequest(TraverseNode *trav);
    void gridPreloadProcess(TraverseNode *root);
    void gridPreloadProcess(TraverseNode *trav,
                            const std::vector<TileId> &requests);
    void resolveBlending(TraverseNode *root,
                CameraMapLayer &layer, float elapsedTime);
    void sortOpaqueFrontToBack();
    void updateCamera(double elapsedTime);
    void suggestedNearFar(double &near_, double &far_);
};

} // namespace vts

#endif
