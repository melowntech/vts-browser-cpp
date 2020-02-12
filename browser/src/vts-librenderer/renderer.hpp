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

#ifndef RENDERER_HPP_deh4f6d4hj
#define RENDERER_HPP_deh4f6d4hj

#include <unordered_map>

#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>
#include <vts-browser/buffer.hpp>

#include "include/vts-renderer/classes.hpp"
#include "include/vts-renderer/renderer.hpp"

#include "shapes.hpp"

namespace vts
{

class CameraDraws;
class DrawSurfaceTask;
class DrawInfographicsTask;

namespace renderer
{

class RenderContextImpl;
class GeodataTile;
struct Text;

// reading depth immediately requires implicit sync between cpu and gpu, which is wasteful
//   instead, this buffer stores the depth into temporary pbo (pixel buffer object)
//   and reads it to cpu in separate step (in next frame)
class DepthBuffer
{
private:
    static const uint32 PboCount = 2;
    Buffer buffer;
    mat4 conv[PboCount];
    uint32 w[PboCount], h[PboCount];
    uint32 pbo[PboCount];
    uint32 tw, th;
    uint32 fbo, tex;
    uint32 index;

    double valuePix(uint32 x, uint32 y);

public:
    DepthBuffer();
    ~DepthBuffer();

    const mat4 &getConv() const;

    void performCopy(uint32 sourceTexture, uint32 w, uint32 h,
        const mat4 &storeConv);

    // xy in -1..1
    // returns 0..1 in logarithmic depth
    double value(double x, double y);

    std::shared_ptr<Shader> shaderCopyDepth;
    std::shared_ptr<Mesh> meshQuad;
};

class ShaderAtm : public Shader
{
public:
    struct AtmBlock
    {
        mat4f uniAtmViewInv;
        vec4f uniAtmColorHorizon;
        vec4f uniAtmColorZenith;
        vec4f uniAtmSizes; // atmosphere thickness (divided by major axis), major / minor axes ratio, inverze major axis
        vec4f uniAtmCoefs; // horizontal exponent, colorGradientExponent
        vec3f uniAtmCameraPosition; // world position of camera (divided by major axis)
        float _dummy; // the AtmBlock may not be smaller than corresponding definition in the shader (WebGL restriction)
    };

    void initializeAtmosphere();
};

struct GeodataJob
{
    std::shared_ptr<GeodataTile> g;
    Rect collisionRect; // ndc space (-1..1)
    Rect iconRect;
    Rect stickRect;
    Rect labelRect;
    vec2f labelOffset;
    vec2f refPoint; // ndc space (-1..1)
    uint32 itemIndex; // -1 means all
    float importance;
    float opacity; // 0..1
    float depth; // ndc space (0..1) - with z-buffer-offset applied
    GeodataJob(const std::shared_ptr<GeodataTile> &g, uint32 itemIndex);
    vec3f modelPosition() const;
    vec3 worldPosition() const;
    vec3f worldUp() const;
};

extern uint32 maxAntialiasingSamples;
extern float maxAnisotropySamples;

void clearGlState();
void enableClipDistance(bool enable);

struct UboCache
{
    std::vector<std::unique_ptr<UniformBuffer>> data;
    uint32 current, last, prev;
    UboCache();
    UniformBuffer *get();
    void frame();
};

class RenderViewImpl
{
public:
    Camera *const camera;
    RenderView *const api;
    RenderContextImpl *const context;

    RenderVariables vars;
    RenderOptions options;
    DepthBuffer depthBuffer;
    UboCache uboCacheSmall;
    UboCache uboCacheLarge;
    std::vector<GeodataJob> geodataJobs;
    std::unordered_map<std::string, GeodataJob> hysteresisJobs;
    CameraDraws *draws;
    const MapCelestialBody *body;
    Texture *atmosphereDensityTexture;
    GeodataTile *lastUboViewPointer;
    mat4 view;
    mat4 viewInv;
    mat4 proj;
    mat4 projInv;
    mat4 viewProj;
    mat4 viewProjInv;
    mat4 davidProj;
    mat4 davidProjInv;
    vec3 zBufferOffsetValues;
    double elapsedTime;
    uint32 width;
    uint32 height;
    uint32 antialiasingPrev;
    uint32 frameIndex;
    uint32 frameRender2BufferId;
    uint32 colorRender2TexId;
    uint32 normalRenderTexId;

    bool projected;
    bool lodBlendingWithDithering;

    RenderViewImpl(Camera *camera, RenderView *api,
        RenderContextImpl *context);

    UniformBuffer *useDisposableUbo(uint32 bindIndex,
        void *data, uint32 size);
    template<class T>
    UniformBuffer *useDisposableUbo(uint32 bindIndex, const T &value)
    { return useDisposableUbo(bindIndex, (void*)&value, sizeof(value)); }

    void drawSurface(const DrawSurfaceTask &t, bool wireframeSlow = false);
    void drawInfographics(const DrawInfographicsTask &t);
    void updateFramebuffers();
    void updateAtmosphereBuffer();
    void getWorldPosition(const double screenPos[2], double worldPos[3]);
    void applyFilter(const std::shared_ptr<Shader> &shader);
    void renderCompass(const double screenPosSize[3],
        const double mapRotation[3]);
    void renderValid();
    void renderEntry();

    bool collides(const GeodataJob &a, const GeodataJob &b);
    bool geodataTestVisibility(
        const float visibility[4],
        const vec3 &pos, const vec3f &up);
    bool geodataDepthVisibility(const vec3 &pos, float threshold);
    mat4 depthOffsetCorrection(const std::shared_ptr<GeodataTile> &g) const;
    void renderGeodataQuad(const GeodataJob &job,
        const Rect &rect, const vec4f &color);
    void bindUboView(const std::shared_ptr<GeodataTile> &gg);
    void computeZBufferOffsetValues();
    void bindUboCamera();
    void renderGeodata();
    void regenerateJobCommon(GeodataJob &j);
    void regenerateJobIcon(GeodataJob &j);
    void regenerateJobStick(GeodataJob &j);
    void regenerateJobCollision(GeodataJob &j);
    bool regenerateJobLabelFlat(GeodataJob &j);
    void regenerateJobLabelScreen(GeodataJob &j);
    bool regenerateJob(GeodataJob &j);
    void generateJobs();
    void sortJobsByZIndexAndImportance();
    void renderJobsDebugRects();
    void renderJobsDebugGlyphs();
    void renderJobsDebugImportance();
    void filterJobsByResolvingCollisions();
    void processJobsHysteresis();
    void sortJobsByZIndexAndDepth();
    void renderStick(const GeodataJob &job);
    void renderPointOrLine(const GeodataJob &job);
    void renderIcon(const GeodataJob &job);
    void renderLabelFlat(const GeodataJob &job);
    void renderLabelScreen(const GeodataJob &job);
    void renderJobs();
};

class RenderContextImpl
{
public:
    RenderContext *const api;

    ContextOptions options;

    std::shared_ptr<Texture> texCompas;
    std::shared_ptr<Texture> texBlueNoise; // uses texture array!
    std::shared_ptr<ShaderAtm> shaderSurface;
    std::shared_ptr<ShaderAtm> shaderSurfaceWithNormalTexure;
    std::shared_ptr<ShaderAtm> shaderBackground;
    std::shared_ptr<Shader> shaderInfographics;
    std::shared_ptr<Shader> shaderTexture;
    std::shared_ptr<Shader> shaderGreyscale;
    std::shared_ptr<Shader> shaderDepth;
    std::shared_ptr<Shader> shaderNormal;
    std::shared_ptr<Shader> shaderDOF;
    std::shared_ptr<Shader> shaderDOF2;
    std::shared_ptr<Shader> shaderFXAA;
    std::shared_ptr<Shader> shaderFXAA2;
    std::shared_ptr<Shader> shaderSSAO;
    std::shared_ptr<Shader> shaderSSAO2;
    std::shared_ptr<Shader> shaderCopyDepth;
    std::shared_ptr<Shader> shaderGeodataColor;
    std::shared_ptr<Shader> shaderGeodataPointFlat;
    std::shared_ptr<Shader> shaderGeodataPointScreen;
    std::shared_ptr<Shader> shaderGeodataLineFlat;
    std::shared_ptr<Shader> shaderGeodataLineScreen;
    std::shared_ptr<Shader> shaderGeodataIconScreen;
    std::shared_ptr<Shader> shaderGeodataLabelFlat;
    std::shared_ptr<Shader> shaderGeodataLabelScreen;
    std::shared_ptr<Shader> shaderGeodataTriangle;
    std::shared_ptr<Mesh> meshQuad; // positions: -1 .. 1
    std::shared_ptr<Mesh> meshRect; // positions: 0 .. 1
    std::shared_ptr<Mesh> meshLine;
    std::shared_ptr<Mesh> meshEmpty;

    RenderContextImpl(RenderContext *api);
    ~RenderContextImpl();
};

} // namespace renderer

} // namespace vts

#endif
