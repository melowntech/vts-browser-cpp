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
#include <vts-browser/celestial.hpp>

#include "geodata.hpp"

namespace vts { namespace renderer
{

GeodataBase::GeodataBase() : renderer(nullptr), info(nullptr)
{}

void GeodataBase::load(RenderContextImpl *renderer, ResourceInfo &info,
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

    copyFonts();

    switch (spec.type)
    {
    case GpuGeodataSpec::Type::LineScreen:
    case GpuGeodataSpec::Type::LineFlat:
        loadLines();
        break;
    case GpuGeodataSpec::Type::PointScreen:
    case GpuGeodataSpec::Type::PointFlat:
        loadPoints();
        break;
    case GpuGeodataSpec::Type::Triangles:
        loadTriangles();
        break;
    case GpuGeodataSpec::Type::PointLabel:
        loadPointLabels();
        break;
    case GpuGeodataSpec::Type::LineLabel:
        loadLineLabels();
        break;
    default:
        throw std::invalid_argument("invalid geodata type");
    }

    // free some memory
    std::vector<std::string>().swap(spec.texts);
    std::vector<std::shared_ptr<void>>().swap(spec.fontCascade);

    // compute memory requirements
    this->info->ramMemoryCost += getTotalPoints()
        * sizeof(decltype(spec.positions[0][0]));
    this->info->ramMemoryCost += spec.iconCoords.size()
        * sizeof(decltype(spec.iconCoords[0]));
    this->info->ramMemoryCost += sizeof(spec) + sizeof(*this);
    this->info = nullptr;
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
    for (const auto &li : spec.positions)
        totalPoints += li.size();
    return totalPoints;
}

Rect::Rect() : a(nan2().cast<float>()), b(nan2().cast<float>())
{}

Rect::Rect(const vec2f &a, const vec2f &b) : a(a), b(b)
{}

bool Rect::valid() const
{
    return a == a && b == b;
}

Rect Rect::merge(const Rect &a, const Rect &b)
{
    if (!a.valid())
        return b;
    if (!b.valid())
        return a;
    Rect r;
    for (int i = 0; i < 2; i++)
    {
        r.a[i] = std::min(a.a[i], b.a[i]);
        r.b[i] = std::max(a.b[i], b.b[i]);
    }
    return r;
}

bool Rect::overlaps(const Rect &a, const Rect &b)
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

float Rect::width() const
{
    return b[0] - a[0];
}

float Rect::height() const
{
    return b[1] - a[1];
}

GeodataJob::GeodataJob(const std::shared_ptr<GeodataBase> &g,
    uint32 itemIndex)
    : g(g), refPoint(0, 0), itemIndex(itemIndex),
    importance(0), opacity(1), depth(nan1())
{}

bool RenderViewImpl::geodataTestVisibility(
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
    if (visibility[3] == visibility[3]
        && dot(vec3((eye - pos) / distance).cast<float>(), up)
            < visibility[3])
        return false;
    vec4 sp = viewProj * vec3to4(pos, 1);
    for (uint32 i = 0; i < 3; i++)
        if (sp[i] < -sp[3] || sp[i] > sp[3])
            return false; // near & far planes culling
    return true;
}

namespace
{
    double raySphereTest(const vec3 &orig, const vec3 &dir, double radius)
    {
        double radius2 = radius * radius;
        vec3 L = -orig;
        double tca = dot(L, dir);
        double d2 = dot(L, L) - tca * tca;
        if (d2 > radius2)
            return nan1();
        double thc = std::sqrt(radius2 - d2);
        double t0 = tca - thc;
        double t1 = tca + thc;
        if (t0 > t1)
            std::swap(t0, t1);
        if (t0 < 0)
        {
            t0 = t1;
            if (t0 < 0)
                return nan1();
        }
        return t0;
    }

    double rayEllipsoidTest(const vec3 &orig, const vec3 &dir,
        double radiusXY, double radiusZ)
    {
        double r = radiusXY / radiusZ;
        vec3 o = vec3(orig[0], orig[1], orig[2] * r); // ellipsoid to sphere
        vec3 d = vec3(dir[0], dir[1], dir[2] * r);
        d = normalize(d);
        double t = raySphereTest(o, d, radiusXY);
        if (!(t == t))
            return nan1();
        vec3 p = o + d * t;
        p = vec3(p[0], p[1], p[2] / r); // sphere to ellipsoid
        return length(vec3(p - o));
    }
}

bool RenderViewImpl::geodataDepthVisibility(const vec3 &pos, float threshold)
{
    if (!(threshold == threshold))
        return true;

    vec3 diff = vec3(rawToVec3(draws->camera.eye) - pos);
    vec3 dir = normalize(diff);

    // if the feature is very far, the depth buffer is not precise enough
    // we compare it with ellipsoid instead
    if (diff.squaredNorm() > 1e13)
    {
        double de = rayEllipsoidTest(rawToVec3(draws->camera.eye),
            -dir, body->majorRadius, body->minorRadius);
        if (!(de == de))
            return true;
        double df = length(diff);
        return df < de + threshold;
    }

    // compare to the depth buffer
    vec3 p3 = pos + dir * threshold;
    vec4 p4 = depthBuffer.getConv() * vec3to4(p3, 1);
    p3 = vec4to3(p4, true);
    double d = depthBuffer.value(p3[0], p3[1]) * 2 - 1;
    if (d == d)
        return p3[2] < d;

    // if the depth value is invalid (eg. sky), the feature is visible
    return true;
}

mat4 RenderViewImpl::depthOffsetCorrection(
    const std::shared_ptr<GeodataBase> &g) const
{
    vec3 zbo = rawToVec3(g->spec.commonData.zBufferOffset).cast<double>();
    double off = dot(zbo, zBufferOffsetValues) * 0.0001;
    double off2 = (off + 1) * 2 - 1;
    mat4 s = scaleMatrix(vec3(1, 1, off2));
    return davidProjInv * s * davidProj;
}

namespace
{

mat4f rectTransform(const Rect &rect, float depth)
{
    vec3 p = vec2to3(vec2((rect.a + rect.b).cast<double>() * 0.5),
        (double)depth);
    vec2 s = vec2((rect.b - rect.a).cast<double>() * 0.5);
    mat4 mvp = translationMatrix(p)
        * scaleMatrix(vec2to3(s, 1));
    return mvp.cast<float>();
}

Rect pointToRect(const vec2f &p, uint32 w, uint32 h)
{
    Rect r;
    r.a = p - vec2f(5.f / w, 5.f / h);
    r.b = p + vec2f(5.f / w, 5.f / h);
    return r;
}

vec2f numericOrigin(GpuGeodataSpec::Origin o)
{
    vec2f origin = vec2f(0.5, 1);
    switch (o)
    {
    case GpuGeodataSpec::Origin::TopLeft:
        origin = vec2f(0, 1);
        break;
    case GpuGeodataSpec::Origin::TopRight:
        origin = vec2f(1, 1);
        break;
    case GpuGeodataSpec::Origin::TopCenter:
        origin = vec2f(0.5, 1);
        break;
    case GpuGeodataSpec::Origin::CenterLeft:
        origin = vec2f(0, 0.5);
        break;
    case GpuGeodataSpec::Origin::CenterRight:
        origin = vec2f(1, 0.5);
        break;
    case GpuGeodataSpec::Origin::CenterCenter:
        origin = vec2f(0.5, 0.5);
        break;
    case GpuGeodataSpec::Origin::BottomLeft:
        origin = vec2f(0, 0);
        break;
    case GpuGeodataSpec::Origin::BottomRight:
        origin = vec2f(1, 0);
        break;
    case GpuGeodataSpec::Origin::BottomCenter:
        origin = vec2f(0.5, 0);
        break;
    default:
        break;
    }
    return origin;
}

} // namespace

void RenderViewImpl::renderGeodataQuad(const Rect &rect,
    float depth, const vec4f &color)
{
    if (!rect.valid())
        return;
    context->shaderGeodataColor->bind();
    mat4f mvpf = rectTransform(rect, depth);
    context->shaderGeodataColor->uniformMat4(0, mvpf.data());
    context->shaderGeodataColor->uniformVec4(1, color.data());
    context->meshQuad->bind();
    context->meshQuad->dispatch();
}

void RenderViewImpl::bindUboView(const std::shared_ptr<GeodataBase> &g)
{
    if (g.get() == lastUboViewPointer)
        return;
    lastUboViewPointer = g.get();

    mat4 model = rawToMat4(g->spec.model);
    mat4 mv = view * model;
    mat4 mvp = proj * depthOffsetCorrection(g) * mv;
    mat4 mvInv = mv.inverse();
    mat4 mvpInv = mvp.inverse();

    // view ubo
    struct UboViewData
    {
        mat4f mvp;
        mat4f mvpInv;
        mat4f mv;
        mat4f mvInv;
    } uboView;

    uboView.mvp = mvp.cast<float>();
    uboView.mvpInv = mvpInv.cast<float>();
    uboView.mv = mv.cast<float>();
    uboView.mvInv = mvInv.cast<float>();

    auto ubo = getUbo();
    ubo->debugId = "UboViewData";
    ubo->bind();
    ubo->load(uboView);
    ubo->bindToIndex(1);
}

void RenderViewImpl::renderGeodata()
{
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    computeZBufferOffsetValues();
    bindUboCamera();
    generateJobs();
    if (options.renderGeodataDebug == 2)
    {
        sortJobsByZIndexAndDepth();
        renderJobsDebugRects();
    }
    sortJobsByZIndexAndImportance();
    if (options.renderGeodataDebug == 1)
        renderJobsDebugImportance();
    filterJobsByResolvingCollisions();
    processJobsHysteresis();
    sortJobsByZIndexAndDepth();
    renderJobs();

    glDepthMask(GL_TRUE);
}

void RenderViewImpl::computeZBufferOffsetValues()
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
    //   and the values already in z-buffer are compatible

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

void RenderViewImpl::bindUboCamera()
{
    struct UboCameraData
    {
        mat4f proj;
        vec4f cameraParams; // screen width in pixels, screen height in pixels, view extent in meters
    } ubo;

    ubo.proj = proj.cast<float>();
    ubo.cameraParams = vec4f(width, height,
        draws->camera.viewExtent, 0);

    uboGeodataCamera->bind();
    uboGeodataCamera->load(ubo);
    uboGeodataCamera->bindToIndex(0);
}

void RenderViewImpl::regenerateJob(GeodataJob &j)
{
    const auto &g = j.g;
    const auto index = j.itemIndex;

    switch (g->spec.type)
    {
    case GpuGeodataSpec::Type::LineScreen:
    case GpuGeodataSpec::Type::LineFlat:
    case GpuGeodataSpec::Type::PointScreen:
    case GpuGeodataSpec::Type::PointFlat:
    case GpuGeodataSpec::Type::Triangles:
    {
        // nothing
    } break;
    case GpuGeodataSpec::Type::PointLabel:
    {
        const auto &t = g->texts[index];

        if (!g->spec.importances.empty())
        {
            j.importance = g->spec.importances[index];
            j.importance
                -= g->spec.commonData.importanceDistanceFactor
                * std::log(length(vec3(t.worldPosition
                    - rawToVec3(draws->camera.eye))));
        }

        const vec4 sp = viewProj * vec3to4(t.worldPosition, 1);

        // ref point
        j.refPoint = vec3to2(vec4to3(sp, true)).cast<float>();

        // depth
        {
            mat4 depthCorrection = proj
                * depthOffsetCorrection(g) * projInv;
            vec4 p = depthCorrection * sp;
            j.depth = p[2] / p[3];
        }

        // label rect
        {
            float sc = options.textScale;
            vec2f ss = t.rectSize.cwiseProduct(
                vec2f(sc / width, sc / height));
            j.labelRect.a = j.labelRect.b = j.refPoint + vec2f(
                g->spec.unionData.pointLabel.offset[0] / width,
                g->spec.unionData.pointLabel.offset[1] / height
            ) - numericOrigin(g->spec.unionData.pointLabel.origin)
                .cwiseProduct(ss);
            j.labelRect.b += ss;
        }

        // icon rect
        {
            const auto &c = g->spec.commonData.icon;
            if (c.scale > 0)
            {
                const auto &s = g->spec.iconCoords[index];
                float w = c.scale * 0.5f * s[4] / width;
                float h = c.scale * 0.5f * s[5] / height;
                j.iconRect.a = j.iconRect.b = j.refPoint;
                j.iconRect.a[0] -= w;
                j.iconRect.b[0] += w;
                j.iconRect.a[1] -= h;
                j.iconRect.b[1] += h;
            }
        }

        // stick
        {
            const auto &s = g->spec.commonData.stick;
            if (s.width > 0)
            {
                float stick = s.heightMax;
                vec3f camForward
                    = vec4to3(vec4(viewInv
                        * vec4(0, 0, -1, 0)), false).cast<float>();
                vec3f camUp
                    = vec4to3(vec4(viewInv
                        * vec4(0, 1, 0, 0)), false).cast<float>();
                vec3f camDir = normalize(rawToVec3(
                    draws->camera.eye)).cast<float>();
                stick *= std::max(0.f,
                    (1 + dot(t.worldUp, camForward))
                    * dot(camUp, camDir)
                );
                if (stick < s.heightThreshold)
                    stick = 0;
                float ro = (stick > 0 ? stick + s.offset : 0)
                    / height;
                j.stickRect.a = j.stickRect.b
                    = vec3to2(vec4to3(sp, true)).cast<float>();
                float w = s.width / width;
                float h = stick / height;
                j.stickRect.a[0] -= w;
                j.stickRect.b[0] += w;
                j.stickRect.b[1] += h;

                // offset others
                j.labelRect.a[1] += h;
                j.labelRect.b[1] += h;
                j.iconRect.a[1] += h;
                j.iconRect.b[1] += h;
            }
        }

        // collision rect
        if (g->spec.commonData.preventOverlap)
            j.collisionRect = Rect::merge(j.labelRect, j.iconRect);
    } break;
    }
}

void RenderViewImpl::generateJobs()
{
    geodataJobs.clear();
    for (const auto &t : draws->geodata)
    {
        std::shared_ptr<GeodataBase> g
            = std::static_pointer_cast<GeodataBase>(t.geodata);

        if (draws->camera.viewExtent < g->spec.commonData.tileVisibility[0]
            || draws->camera.viewExtent >= g->spec.commonData.tileVisibility[1])
            continue;

        switch (g->spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineFlat:
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointFlat:
        case GpuGeodataSpec::Type::Triangles:
        {
            // one job for entire tile
            geodataJobs.emplace_back(g, uint32(-1));
        } break;
        case GpuGeodataSpec::Type::PointLabel:
        {
            if (!g->checkTextures())
                continue;

            // individual jobs for each text
            for (uint32 index = 0, indexEnd = g->texts.size();
                index < indexEnd; index++)
            {
                const auto &t = g->texts[index];

                if (!geodataTestVisibility(
                    g->spec.commonData.visibilities,
                    t.worldPosition, t.worldUp))
                    continue;

                if (!geodataDepthVisibility(t.worldPosition,
                    g->spec.commonData.depthVisibilityThreshold))
                    continue;

                GeodataJob j(g, index);
                regenerateJob(j);
                geodataJobs.push_back(std::move(j));
            }
        } break;
        default:
        {
        } break;
        }
    }
}

void RenderViewImpl::sortJobsByZIndexAndImportance()
{
    // primary: z-index
    // secondary: importance
    std::sort(geodataJobs.begin(), geodataJobs.end(),
        [](const GeodataJob &a, const GeodataJob &b)
        {
            auto az = a.g->spec.commonData.zIndex;
            auto bz = b.g->spec.commonData.zIndex;
            if (az == bz)
                return a.importance > b.importance;
            return az < bz;
        });
}

void RenderViewImpl::renderJobsDebugRects()
{
    vec4f colorCollision = vec4f(0.5, 0.5, 0.5, 0.4);
    vec4f colorStick = vec4f(0, 0, 0.5, 0.4);
    vec4f colorIcon = vec4f(0, 0.5, 0, 0.4);
    vec4f colorLabel = vec4f(0.5, 0, 0, 0.4);
    vec4f colorRef = vec4f(1, 0, 1, 1);
    for (const GeodataJob &job : geodataJobs)
    {
        if (!job.labelRect.valid())
            continue;
        renderGeodataQuad(job.collisionRect, job.depth, colorCollision);
        renderGeodataQuad(job.stickRect, job.depth, colorStick);
        renderGeodataQuad(job.iconRect, job.depth, colorIcon);
        renderGeodataQuad(job.labelRect, job.depth, colorLabel);
        renderGeodataQuad(pointToRect(job.refPoint, width, height),
            job.depth, colorRef);
    }
}

void RenderViewImpl::renderJobsDebugImportance()
{
    uint32 cnt = geodataJobs.size();
    struct Quad
    {
        Rect r;
        vec4f c;
        float z;
        Quad(const Rect &r, const vec4f &c, float z) : r(r), c(c), z(z)
        {}
    };
    std::vector<Quad> quads;
    quads.reserve(cnt);
    uint32 i = 0;
    for (const GeodataJob &job : geodataJobs)
    {
        if (!job.labelRect.valid())
            continue;
        vec3f color = convertToRainbowColor(1 - float(i++) / cnt);
        quads.emplace_back(job.labelRect,
            vec3to4(color, 0.35f), job.depth);
    }
    std::sort(quads.begin(), quads.end(),
        [](const Quad &a, const Quad &b)
        {
            return a.z > b.z;
        });
    for (const Quad &a : quads)
        renderGeodataQuad(a.r, a.z, a.c);
}

void RenderViewImpl::filterJobsByResolvingCollisions()
{
    float pixels = width * height;
    uint32 index = 0;
    std::vector<Rect> rects;
    rects.reserve(geodataJobs.size());
    geodataJobs.erase(std::remove_if(geodataJobs.begin(),
        geodataJobs.end(), [&](const GeodataJob &l) {
        float limitFactor = l.g->spec.commonData.featuresLimitPerPixelSquared;
        if (index > limitFactor * pixels)
            return true;
        if (l.collisionRect.valid())
        {
            const Rect mr = l.collisionRect;
            for (const Rect &r : rects)
                if (Rect::overlaps(mr, r))
                    return true;
            rects.push_back(mr);
        }
        if (limitFactor == limitFactor)
            index++;
        return false;
    }), geodataJobs.end());
}

void RenderViewImpl::processJobsHysteresis()
{
    if (!options.geodataHysteresis)
    {
        hysteresisJobs.clear();
        return;
    }

    for (auto it = hysteresisJobs.begin(); it != hysteresisJobs.end(); )
    {
        it->second.opacity -= elapsedTime
            / it->second.g->spec.commonData.hysteresisDuration[1];
        if (it->second.opacity < -0.5f)
            it = hysteresisJobs.erase(it);
        else
            it++;
    }

    for (auto &it : geodataJobs)
    {
        if (it.itemIndex == (uint32)-1 || it.g->spec.hysteresisIds.empty())
            continue;
        const std::string &id = it.g->spec.hysteresisIds[it.itemIndex];
        auto hit = hysteresisJobs.find(id);
        if (hit == hysteresisJobs.end())
            it.opacity = -0.5f;
        else
        {
            it.opacity = hit->second.opacity;
            hysteresisJobs.erase(hit);
        }
        it.opacity +=
            + elapsedTime / it.g->spec.commonData.hysteresisDuration[0]
            + elapsedTime / it.g->spec.commonData.hysteresisDuration[1];
        it.opacity = std::min(it.opacity, 1.f);
    }

    for (auto &it : hysteresisJobs)
    {
        if (it.second.opacity > 0.f)
        {
            regenerateJob(it.second);
            geodataJobs.push_back(it.second);
        }
    }

    hysteresisJobs.clear();
    geodataJobs.erase(std::remove_if(geodataJobs.begin(),
        geodataJobs.end(), [&](GeodataJob &it) {
        if (it.itemIndex == (uint32)-1 || it.g->spec.hysteresisIds.empty())
            return false;
        const std::string &id = it.g->spec.hysteresisIds[it.itemIndex];
        hysteresisJobs.insert(std::make_pair(id, it));
        return it.opacity <= 0;
    }), geodataJobs.end());
}

void RenderViewImpl::sortJobsByZIndexAndDepth()
{
    std::sort(geodataJobs.begin(), geodataJobs.end(),
        [](const GeodataJob &a, const GeodataJob &b)
        {
            auto az = a.g->spec.commonData.zIndex;
            auto bz = b.g->spec.commonData.zIndex;
            if (az == bz)
                return a.depth > b.depth;
            return az < bz;
        });
}

void RenderViewImpl::renderStick(const GeodataJob &job)
{
    const auto &s = job.g->spec.commonData.stick;
    vec4f color = rawToVec4(job.g->spec.commonData.stick.color);
    color[3] *= job.opacity;
    renderGeodataQuad(job.stickRect, job.depth, color);
}

void RenderViewImpl::renderIcon(const GeodataJob &job,
    const float uvs[4])
{
    struct UboIcon
    {
        mat4f mvp;
        vec4f params;
        vec4f uvs;
    } uboIcon;

    const auto &icon = job.g->spec.commonData.icon;

    uboIcon.mvp = rectTransform(job.iconRect, job.depth);
    uboIcon.params = vec4f(job.opacity, 0, 0, 0);
    uboIcon.uvs = rawToVec4(uvs);

    auto uboGeodataIcon = getUbo();
    uboGeodataIcon->debugId = "UboIcon";
    uboGeodataIcon->bind();
    uboGeodataIcon->load(uboIcon);
    uboGeodataIcon->bindToIndex(2);

    ((Texture*)job.g->spec.bitmap.get())->bind();

    context->shaderGeodataIcon->bind();
    context->meshQuad->bind();
    context->meshQuad->dispatch();
}

void RenderViewImpl::renderLabel(const GeodataJob &job)
{
    struct UboText
    {
        vec4f color[2];
        vec4f outline;
        vec4f position; // xyz, scale
        vec4f offset;
        vec4f coordinates[1000];
    } uboText;

    const auto &g = job.g;
    const auto &t = g->texts[job.itemIndex];

    uboText.color[0] = rawToVec4(g->spec.unionData.pointLabel.color);
    uboText.color[1] = rawToVec4(g->spec.unionData.pointLabel.color2);
    uboText.color[0][3] *= job.opacity;
    uboText.color[1][3] *= job.opacity;
    uboText.outline = g->outline;
    uboText.position[0] = t.modelPosition[0];
    uboText.position[1] = t.modelPosition[1];
    uboText.position[2] = t.modelPosition[2];
    uboText.position[3] = options.textScale;
    vec2f off = (job.labelRect.a + job.labelRect.b) * 0.5
        - job.refPoint;
    uboText.offset = vec4f(off[0], off[1], 0, 0);

    {
        vec4f *o = uboText.coordinates;
        for (const vec4f &c : t.coordinates)
            *o++ = c;
    }

    context->shaderGeodataPointLabel->bind();
    auto uboGeodataText = getUbo();
    uboGeodataText->debugId = "UboText";
    uboGeodataText->bind();
    uboGeodataText->load(&uboText,
        20 * sizeof(float) + 4 * sizeof(float) * t.coordinates.size());
    uboGeodataText->bindToIndex(2);

    context->meshEmpty->bind();
    for (int pass = 0; pass < 2; pass++)
    {
        context->shaderGeodataPointLabel->uniform(0, pass);
        for (auto &w : t.words)
        {
            w.texture->bind();
            context->meshEmpty->dispatch(
                w.coordinatesStart, w.coordinatesCount);
        }
    }
}

void RenderViewImpl::renderJobs()
{
    for (const GeodataJob &job : geodataJobs)
    {
        const auto &g = job.g;

        switch (g->spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineFlat:
        {
            assert(job.itemIndex == (uint32)-1);
            bindUboView(g);
            context->shaderGeodataLine->bind();
            g->uniform->bindToIndex(2);
            g->texture->bind();
            Mesh *msh = g->mesh.get();
            msh->bind();
            glEnable(GL_STENCIL_TEST);
            msh->dispatch();
            glDisable(GL_STENCIL_TEST);
        } break;
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointFlat:
        {
            assert(job.itemIndex == (uint32)-1);
            bindUboView(g);
            context->shaderGeodataPoint->bind();
            g->uniform->bindToIndex(2);
            g->texture->bind();
            Mesh *msh = g->mesh.get();
            msh->bind();
            glEnable(GL_STENCIL_TEST);
            msh->dispatch();
            glDisable(GL_STENCIL_TEST);

            /*
            if (job.stick > 0 || g->spec.commonData.icon.scale > 0)
            {
                for (uint32 index = 0, indexEnd = g->spec.positions.size();
                    index < indexEnd; index++)
                {
                    vec3 worldPosition = vec4to3(vec4(g->model * vec3to4(
                        rawToVec3(g->spec.positions[index][0].data()),
                            1).cast<double>()));
                    renderStick(job, worldPosition);
                    renderIcon(job, worldPosition);
                }
            }
            */
        } break;
        case GpuGeodataSpec::Type::Triangles:
        {
            assert(job.itemIndex == (uint32)-1);
            bindUboView(g);
            context->shaderGeodataTriangle->bind();
            g->uniform->bindToIndex(2);
            Mesh *msh = g->mesh.get();
            msh->bind();
            bool stencil = g->spec.unionData.triangles.useStencil;
            if (stencil)
                glEnable(GL_STENCIL_TEST);
            glDepthMask(GL_TRUE);
            msh->dispatch();
            glDepthMask(GL_FALSE);
            if (stencil)
                glDisable(GL_STENCIL_TEST);
        } break;
        case GpuGeodataSpec::Type::PointLabel:
        {
            const auto &t = g->texts[job.itemIndex];

            if (!geodataTestVisibility(
                g->spec.commonData.visibilities,
                t.worldPosition, t.worldUp))
                continue;

            bindUboView(g);
            renderStick(job);
            if (job.g->spec.commonData.icon.scale > 0)
                renderIcon(job, job.g->spec.iconCoords[job.itemIndex].data());
            renderLabel(job);
        } break;
        default:
        {
        } break;
        }
    }

    geodataJobs.clear();

    lastUboViewPointer = nullptr;
}

void RenderContext::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec,
    const std::string &debugId)
{
    auto r = std::make_shared<GeodataBase>();
    r->load(&*impl, info, spec, debugId);
    info.userData = r;
}

} } // namespace vts renderer

