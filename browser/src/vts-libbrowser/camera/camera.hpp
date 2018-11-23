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

namespace vts
{

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
    void renderNodeCoarser(TraverseNode *trav,
                    TraverseNode *orig, vec4f uvClip);
    void renderNodeCoarser(TraverseNode *trav);
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
    void gridPreloadProcess();
    void gridPreloadProcess(TraverseNode *trav,
                            const std::vector<TileId> &requests);
    void sortOpaqueFrontToBack();
    void updateCamera(double elapsedTime);
    void suggestedNearFar(double &near_, double &far_);
};

} // namespace vts

#endif
