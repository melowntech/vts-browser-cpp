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

#include <memory>
#include <vector>
#include <unordered_map>
#include <map>

#include <vts-libs/registry/referenceframe.hpp>

#include "include/vts-browser/cameraCredits.hpp"
#include "include/vts-browser/cameraDraws.hpp"
#include "include/vts-browser/cameraOptions.hpp"
#include "include/vts-browser/cameraStatistics.hpp"
#include "include/vts-browser/math.hpp"

namespace vtslibs { namespace vts {
class NodeInfo;
} }

namespace vts
{

enum class Validity;

class MapImpl;
class Camera;
class TraverseNode;
class NavigationImpl;
class RenderSurfaceTask;
class RenderInfographicsTask;
class RenderColliderTask;
class GpuTexture;
class DrawSurfaceTask;
class DrawGeodataTask;
class DrawInfographicsTask;
class DrawColliderTask;
class MapLayer;
class BoundParamInfo;

using vtslibs::vts::NodeInfo;
using TileId = vtslibs::registry::ReferenceFrame::Division::Node::Id;

class BlendDraw
{
public:
    TileId id;
    double age = 0;

    BlendDraw(const TraverseNode *current);
    BlendDraw(const TileId &id);
};

class CameraMapLayer
{
public:
    std::vector<BlendDraw> blendDraws;
};

class CameraImpl : private Immovable
{
public:
    MapImpl *const map = nullptr;
    Camera *const camera = nullptr;
    std::weak_ptr<NavigationImpl> navigation;
    CameraCredits credits;
    CameraDraws draws;
    CameraOptions options;
    CameraStatistics statistics;
    std::vector<TileId> preloadRequests;
    std::vector<TraverseNode*> currentDraws;
    std::map<std::weak_ptr<MapLayer>, CameraMapLayer,
            std::owner_less<std::weak_ptr<MapLayer>>> layers;
    // *Actual = corresponds to current camera settings
    // *Render, *Culling, updated only when camera is NOT detached
    mat4 viewProjActual;
    mat4 viewProjRender;
    mat4 viewProjCulling;
    mat4 viewActual;
    mat4 apiProj;
    vec4 cullingPlanes[6];
    vec3 perpendicularUnitVector;
    vec3 forwardUnitVector;
    vec3 cameraPosPhys;
    vec3 focusPosPhys;
    vec3 eye, target, up;
    double diskNominalDistance = 0;
    uint32 windowWidth = 0;
    uint32 windowHeight = 0;

    CameraImpl(MapImpl *map, Camera *cam);
    void clear();
    Validity reorderBoundLayers(const NodeInfo &nodeInfo, uint32 subMeshIndex,
        std::vector<BoundParamInfo> &boundList,
        double priority);
    void touchDraws(TraverseNode *trav);
    bool visibilityTest(TraverseNode *trav);
    bool coarsenessTest(TraverseNode *trav);
    double coarsenessValue(TraverseNode *trav);
    float getTextSize(float size, const std::string &text);
    void renderText(TraverseNode *trav, float x, float y, const vec4f &color,
                   float size, const std::string &text, bool centerText = true);
    void renderNodeBox(TraverseNode *trav, const vec4f &color);
    void renderNode(TraverseNode *trav);
    void renderNodeBlended(TraverseNode *trav, float blendingCoverage);
    void renderNodePrefill(TraverseNode *trav);
    DrawSurfaceTask convert(const RenderSurfaceTask &task,
                            float blendingCoverage = 1);
    DrawInfographicsTask convert(const RenderInfographicsTask &task);
    DrawColliderTask convert(const RenderColliderTask &task);
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
    void travModeStable(TraverseNode *trav);
    void travModePrefill(TraverseNode *trav);
    void travModeFixed(TraverseNode *trav);
    void traverseRender(TraverseNode *trav, TraverseMode mode);
    void preloadRequest(TraverseNode *trav);
    void preloadProcess(TraverseNode *root);
    void preloadProcess(TraverseNode *trav,
        const std::vector<TileId> &requests);
    void resolveBlending(TraverseNode *root, CameraMapLayer &layer);
    void sortOpaqueFrontToBack();
    void renderUpdate();
    void suggestedNearFar(double &near_, double &far_);
    bool getSurfaceOverEllipsoid(double &result, const vec3 &navPos,
        double sampleSize = -1, bool renderDebug = false);
    double getSurfaceAltitudeSamples();
};

void updateNavigation(std::weak_ptr<NavigationImpl> &nav, double elapsedTime);

} // namespace vts

#endif
