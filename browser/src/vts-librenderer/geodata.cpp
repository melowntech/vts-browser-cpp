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

#include <vts-browser/cameraDraws.hpp>

#include "geodata.hpp"

namespace vts { namespace renderer
{

GeodataBase::GeodataBase() : renderer(nullptr), info(nullptr)
{}

GeodataBase::~GeodataBase()
{}

void GeodataBase::loadInit(RendererImpl *renderer, ResourceInfo &info,
    GpuGeodataSpec &specp, const std::string &debugId)
{
    this->debugId = debugId;

    this->spec = std::move(specp);
    this->info = &info;
    this->renderer = renderer;

    model = rawToMat4(spec.model);
    modelInv = model.inverse();

    {
        // culling (degrees to dot)
        float &c = this->spec.commonData.visibilities[3];
        if (c == c)
            c = std::cos(c * M_PI / 180.0);
    }
}

void GeodataBase::loadFinish()
{
    // free some memory
    std::vector<std::vector<std::array<float, 3>>>()
        .swap(spec.positions);
    std::vector<std::vector<std::array<float, 2>>>()
        .swap(spec.uvs);
    std::vector<std::shared_ptr<void>>().swap(spec.bitmaps);
    std::vector<std::string>().swap(spec.texts);
    std::vector<std::shared_ptr<void>>().swap(spec.fontCascade);

    info->ramMemoryCost += sizeof(spec);
    info = nullptr;
    renderer = nullptr;

    CHECK_GL("load geodata");
}

void GeodataBase::addMemory(ResourceInfo &other)
{
    info->ramMemoryCost += other.ramMemoryCost;
    info->gpuMemoryCost += other.gpuMemoryCost;
}

vec3f GeodataBase::modelUp(const vec3f &p) const
{
    vec3 dl = p.cast<double>();
    vec3 dw = vec4to3(vec4(model * vec3to4(dl, 1)));
    vec3 upw = normalize(dw);
    vec3 tw = dw + upw;
    vec3 tl = vec4to3(vec4(modelInv * vec3to4(tw, 1)));
    return (tl - dl).cast<float>();
}

vec3f GeodataBase::worldUp(vec3f &p) const
{
    vec3 dl = p.cast<double>();
    vec3 dw = vec4to3(vec4(model * vec3to4(dl, 1)));
    vec3 upw = normalize(dw);
    return upw.cast<float>();
}

uint32 GeodataBase::getTotalPoints() const
{
    uint32 totalPoints = 0;
    uint32 linesCount = spec.positions.size();
    for (uint32 li = 0; li < linesCount; li++)
        totalPoints += spec.positions[li].size();
    return totalPoints;
}

bool RendererImpl::geodataTestVisibility(
    const float visibility[4],
    const vec3 &pos, const vec3f &up)
{
    vec3 eye = rawToVec3(draws->camera.eye);
    double distance = length(vec3(eye - pos));
    if (visibility[0] == visibility[0] && distance > visibility[0])
        return false;
    distance *= 2 / draws->camera.proj[5];
    if (visibility[1] == visibility[1] && distance < visibility[1])
        return false;
    if (visibility[2] == visibility[2] && distance > visibility[2])
        return false;
    vec3f f = normalize(vec3(eye - pos)).cast<float>();
    if (visibility[3] == visibility[3]
        && dot(f, up) < visibility[3])
        return false;
    return true;
}

void RendererImpl::initializeGeodata()
{
    // load shader geodata line
    {
        shaderGeodataLine = std::make_shared<Shader>();
        shaderGeodataLine->debugId
            = "data/shaders/geodataLine.*.glsl";
        shaderGeodataLine->loadInternal(
            "data/shaders/geodataLine.vert.glsl",
            "data/shaders/geodataLine.frag.glsl");
        shaderGeodataLine->bindTextureLocations({
                { "texLineData", 0 }
            });
        shaderGeodataLine->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboLineData", 2 }
            });
    }

    // load shader geodata point
    {
        shaderGeodataPoint = std::make_shared<Shader>();
        shaderGeodataPoint->debugId
            = "data/shaders/geodataPoint.*.glsl";
        shaderGeodataPoint->loadInternal(
            "data/shaders/geodataPoint.vert.glsl",
            "data/shaders/geodataPoint.frag.glsl");
        shaderGeodataPoint->bindTextureLocations({
                { "texPointData", 0 }
            });
        shaderGeodataPoint->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboPointData", 2 }
            });
    }

    // load shader geodata point label
    {
        shaderGeodataPointLabel = std::make_shared<Shader>();
        shaderGeodataPointLabel->debugId
            = "data/shaders/geodataPointLabel.*.glsl";
        shaderGeodataPointLabel->loadInternal(
            "data/shaders/geodataPointLabel.vert.glsl",
            "data/shaders/geodataPointLabel.frag.glsl");
        shaderGeodataPointLabel->loadUniformLocations({
                "uniPosition",
                "uniScale",
                "uniPass"
            });
        shaderGeodataPointLabel->bindTextureLocations({
                { "texGlyphs", 0 }
            });
        shaderGeodataPointLabel->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboPointLabelData", 2 }
            });
    }

    uboGeodataCamera = std::make_shared<UniformBuffer>();
    uboGeodataCamera->debugId = "uboGeodataCamera";
    uboGeodataView = std::make_shared<UniformBuffer>();
    uboGeodataView->debugId = "uboGeodataView";

    CHECK_GL("initialize geodata");
}

void RendererImpl::initializeZBufferOffsetValues()
{
    vec3 up1 = normalize(rawToVec3(draws->camera.eye)); // todo projected systems
    vec3 up2 = vec4to3(vec4(viewInv * vec4(0, 1, 0, 0)));
    double tiltFactor = std::acos(std::max(
        dot(up1, up2), 0.0)) / M_PI_2;
    double distance = draws->camera.tagretDistance;
    double distanceFactor = 1 / std::max(1.0,
        std::log(distance) / std::log(1.04));
    zBufferOffsetValues = vec3(1, distanceFactor, tiltFactor);

    // here comes the anti-david trick
    // note: David is the developer behind vts-browser-js
    //          he also designed the geodata styling rules
    // unfortunately, he designed zbuffer-offset in a way
    //   that only works with his perspective projection
    // therefore we simulate his projection,
    //   apply the offset and undo his projection
    // this way the parameters work as expected
    //   and the values already in z-buffer are valid too

    double factor = std::max(draws->camera.altitudeOverEllipsoid,
        draws->camera.tagretDistance) / 600000;
    double davidNear = std::max(2.0, factor * 40);
    double davidFar = 600000 * std::max(1.0, factor) * 20;

    // this decomposition works with the most basic projection matrix only
    double fov = 2 * radToDeg(std::atan(1 / proj(1, 1)));
    double aspect = proj(1, 1) / proj(0, 0);
    double myNear = proj(2, 3) / (proj(2, 2) - 1);
    double myFar = proj(2, 3) / (proj(2, 2) + 1);
    (void)myNear;
    (void)myFar;

    assert(perspectiveMatrix(fov, aspect,
        myNear, myFar).isApprox(proj, 1e-10));

    davidProj = perspectiveMatrix(fov, aspect,
        davidNear, davidFar);
    davidProjInv = davidProj.inverse();
}

void RendererImpl::initializeCameraDataUbo()
{
    struct UboCameraData
    {
        mat4f proj;
        vec4f cameraParams; // screen width in pixels, screen height in pixels, view extent in meters
    } ubo;

    ubo.proj = proj.cast<float>();
    ubo.cameraParams = vec4f(widthPrev, heightPrev,
        draws->camera.viewExtent, 0);

    uboGeodataCamera->bind();
    uboGeodataCamera->load(ubo);
    uboGeodataCamera->bindToIndex(0);
}

mat4 RendererImpl::depthOffsetProj(GeodataBase *gg) const
{
    vec3 zbo = rawToVec3(gg->spec.commonData.zBufferOffset)
        .cast<double>();
    double off = dot(zbo, zBufferOffsetValues) * 0.0001;
    double off2 = (off + 1) * 2 - 1;
    mat4 s = scaleMatrix(vec3(1, 1, off2));
    return proj * davidProjInv * s * davidProj;
}

void RendererImpl::initializeViewDataUbo(GeodataBase *gg)
{
    mat4 model = rawToMat4(gg->spec.model);
    mat4 mv = view * model;
    mat4 mvp = depthOffsetProj(gg) * mv;
    mat4 mvInv = mv.inverse();
    mat4 mvpInv = mvp.inverse();

    // view ubo
    struct UboViewData
    {
        mat4f mvp;
        mat4f mvpInv;
        mat4f mv;
        mat4f mvInv;
    } ubo;

    ubo.mvp = mvp.cast<float>();
    ubo.mvpInv = mvpInv.cast<float>();
    ubo.mv = mv.cast<float>();
    ubo.mvInv = mvInv.cast<float>();

    uboGeodataView->bind();
    uboGeodataView->load(ubo);
    uboGeodataView->bindToIndex(1);
}

void RendererImpl::renderGeodata()
{
    //glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initializeZBufferOffsetValues();
    initializeCameraDataUbo();

    std::sort(draws->geodata.begin(), draws->geodata.end(),
        [](const DrawGeodataTask &a, const DrawGeodataTask &b)
    {
        return ((GeodataBase*)a.geodata.get())->spec.commonData.zIndex
            < ((GeodataBase*)b.geodata.get())->spec.commonData.zIndex;
    });

    // iterate over geodata
    for (const DrawGeodataTask &t : draws->geodata)
    {
        GeodataBase *gg = (GeodataBase*)t.geodata.get();

        switch (gg->spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineFlat:
        {
            initializeViewDataUbo(gg);
            GeodataGeometry *g = static_cast<GeodataGeometry*>(gg);
            shaderGeodataLine->bind();
            g->uniform->bindToIndex(2);
            g->texture->bind();
            Mesh *msh = g->mesh.get();
            msh->bind();
            glEnable(GL_STENCIL_TEST);
            msh->dispatch();
            glDisable(GL_STENCIL_TEST);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            //msh->dispatch();
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } break;
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointFlat:
        {
            initializeViewDataUbo(gg);
            GeodataGeometry *g = static_cast<GeodataGeometry*>(gg);
            shaderGeodataPoint->bind();
            g->uniform->bindToIndex(2);
            g->texture->bind();
            Mesh *msh = g->mesh.get();
            msh->bind();
            glEnable(GL_STENCIL_TEST);
            msh->dispatch();
            glDisable(GL_STENCIL_TEST);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            //msh->dispatch();
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } break;
        case GpuGeodataSpec::Type::PointLabel:
        {
            GeodataText *g = static_cast<GeodataText*>(gg);
            if (!g->checkTextures())
                continue;
            for (auto &t : g->texts)
            {
                if (!geodataTestVisibility(
                    g->spec.commonData.visibilities,
                    t.worldPosition, t.worldUp))
                    continue;
                if (options.renderTextMargins)
                    renderTextMargin(t);
                pointLabelsArray.emplace_back(g, &t);
            }
        } break;
        default:
        {
        } break;
        }
    }

    filterOverlappingLabels();

    glEnable(GL_BLEND);
    renderPointLabels();
    glDisable(GL_BLEND);

    glDepthMask(GL_TRUE);
    //glEnable(GL_CULL_FACE);
}

void RendererImpl::filterOverlappingLabels()
{
    struct Rect
    {
        vec2f a, b;

        Rect(const RendererImpl *impl, const Label &l)
        {
            vec2f ss = vec2f(1.f / impl->widthPrev, 1.f / impl->heightPrev);
            vec4 sp = impl->viewProj * vec3to4(l.t->worldPosition, 1);
            auto p = [&](const vec2f &a) -> vec2f
            {
                vec4 r = sp;
                for (int i = 0; i < 2; i++)
                    r[i] += impl->options.textScale * r[3] * a[i] * ss[i];
                return vec3to2(vec4to3(r, true)).cast<float>();
            };
            a = p(l.t->rectOrigin);
            b = p(l.t->rectOrigin + l.t->rectSize);
        }

        static bool overlaps(const Rect &a, const Rect &b)
        {
            for (int i = 0; i < 2; i++)
            {
                if (a.b[i] < b.a[i])
                    return false;
                if (a.a[i] > b.b[i])
                    return false;
            }
            return true;
        }
    };

    std::vector<Rect> rects;
    rects.reserve(pointLabelsArray.size());

    pointLabelsArray.erase(std::remove_if(pointLabelsArray.begin(),
        pointLabelsArray.end(), [&](const Label &l) {
        Rect mr = Rect(this, l);
        for (const Rect &r : rects)
            if (Rect::overlaps(mr, r))
                return true;
        rects.push_back(mr);
        return false;
    }), pointLabelsArray.end());
}

void RendererImpl::renderTextMargin(const Text &t)
{
    vec2f ss = vec2f(1.f / widthPrev, 1.f / heightPrev);
    vts::DrawSimpleTask st;
    st.mesh = meshLine;
    vecToRaw(vec4f(1, 0, 0, 1), st.color);
    vec4 sp = viewProj * vec3to4(t.worldPosition, 1);
    auto p = [&](const vec2f &a)
    {
        vec4 r = sp;
        for (int i = 0; i < 2; i++)
            r[i] += options.textScale * r[3] * a[i] * ss[i];
        r = viewProjInv * r;
        return vec4to3(r, true);
    };
    auto r = [&](const vec2f &aa, const vec2f &bb)
    {
        vec2f a2 = t.rectOrigin + t.rectSize.cwiseProduct(aa);
        vec2f b2 = t.rectOrigin + t.rectSize.cwiseProduct(bb);
        vec3 a = p(a2);
        vec3 b = p(b2);
        mat4f mv = (view * lookAt(a, b)).cast<float>();
        matToRaw(mv, st.mv);
        draws->infographics.push_back(st);
    };
    r(vec2f(0, 0), vec2f(0, 1));
    r(vec2f(0, 1), vec2f(1, 1));
    r(vec2f(1, 1), vec2f(1, 0));
    r(vec2f(1, 0), vec2f(0, 0));
}

void RendererImpl::renderPointLabels()
{
    std::shared_ptr<Texture> lastTexture;
    GeodataText *lastGeodata = nullptr;
    shaderGeodataPointLabel->bind();
    shaderGeodataPointLabel->uniform(1, options.textScale);
    for (Label &l : pointLabelsArray)
    {
        GeodataText *g = l.g;
        Text &t = *l.t;
        if (g != lastGeodata)
        {
            initializeViewDataUbo(g);
            lastGeodata = g;
        }
        g->uniform->bindToIndex(2);
        shaderGeodataPointLabel->uniformVec3(0,
            t.modelPosition.data());
        for (int pass = 0; pass < 2; pass++)
        {
            shaderGeodataPointLabel->uniform(2, pass);
            for (auto &w : t.words)
            {
                if (w.texture != lastTexture)
                {
                    w.texture->bind();
                    lastTexture = w.texture;
                }
                w.mesh->bind();
                w.mesh->dispatch();
            }
        }
    }
    pointLabelsArray.clear();
}

void Renderer::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec,
    const std::string &debugId)
{
    switch (spec.type)
    {
    case GpuGeodataSpec::Type::PointLabel:
    case GpuGeodataSpec::Type::LineLabel:
    {
        auto r = std::make_shared<GeodataText>();
        r->load(&*impl, info, spec, debugId);
        info.userData = r;
    } break;
    default:
    {

        auto r = std::make_shared<GeodataGeometry>();
        r->load(&*impl, info, spec, debugId);
        info.userData = r;
    } break;
    }
}

} } // namespace vts renderer

