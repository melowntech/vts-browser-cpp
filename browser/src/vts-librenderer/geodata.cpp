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

#include <vts-browser/celestial.hpp>

#include <optick.h>

#include "geodata.hpp"

namespace vts { namespace renderer
{

GeodataTile::GeodataTile() : renderer(nullptr), info(nullptr)
{}

void GeodataTile::load(RenderContextImpl *renderer, ResourceInfo &info,
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
        float &c = spec.commonData.visibilities[3];
        if (!std::isnan(c))
            c = std::cos(c * M_PI / 180.0);
    }

    switch (spec.type)
    {
    case GpuGeodataSpec::Type::PointFlat:
    case GpuGeodataSpec::Type::PointScreen:
        loadPoints();
        break;
    case GpuGeodataSpec::Type::LineFlat:
    case GpuGeodataSpec::Type::LineScreen:
        loadLines();
        break;
    case GpuGeodataSpec::Type::IconFlat:
    case GpuGeodataSpec::Type::IconScreen:
        loadIcons();
        break;
    case GpuGeodataSpec::Type::LabelFlat:
        loadLabelFlats();
        break;
    case GpuGeodataSpec::Type::LabelScreen:
        loadLabelScreens();
        break;
    case GpuGeodataSpec::Type::Triangles:
        loadTriangles();
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

void GeodataTile::addMemory(ResourceInfo &other)
{
    info->ramMemoryCost += other.ramMemoryCost;
    info->gpuMemoryCost += other.gpuMemoryCost;
}

uint32 GeodataTile::getTotalPoints() const
{
    uint32 totalPoints = 0;
    for (const auto &li : spec.positions)
        totalPoints += li.size();
    return totalPoints;
}

vec3f GeodataTile::modelUp(const vec3f &modelPos)
{
    vec3 mp3 = modelPos.cast<double>();
    vec4 mp4 = vec3to4(mp3, 1);
    vec4 wp4 = model * mp4;
    vec3 wp3 = vec4to3(wp4); // no perspective, no division
    vec3 wn3 = normalize(wp3);
    vec4 wn4 = vec3to4(wn3, 0);
    vec4 mn4 = modelInv * wn4;
    vec3 mn3 = vec4to3(mn4); // no division
    return normalize(mn3).cast<float>();
}

void GeodataTile::copyPoints()
{
    mat4 model = rawToMat4(spec.model);
    assert(points.empty());
    points.reserve(spec.positions.size());
    for (const auto &it : spec.positions)
    {
        Point t;
        t.modelPosition = rawToVec3(it[0].data());
        t.worldPosition = vec4to3(vec4(model
            * vec3to4(t.modelPosition, 1).cast<double>()));
        t.worldUp = normalize(t.worldPosition).cast<float>();
        points.push_back(t);
    }
}

Rect::Rect() : a(nan2().cast<float>()), b(nan2().cast<float>())
{}

Rect::Rect(const vec2f &a, const vec2f &b) : a(a), b(b)
{}

bool Rect::valid() const
{
    return !std::isnan(a[0]) && !std::isnan(a[1])
        && !std::isnan(b[0]) && !std::isnan(b[1]);
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

bool overlaps(const GeodataJob &a, const GeodataJob &b)
{
    if (!Rect::overlaps(a.collisionRect, b.collisionRect))
        return false;

    const auto &getRects = [](const GeodataJob &a) -> const std::vector<Rect>&
    {
        static const std::vector<Rect> empty;
        if (a.g->texts.empty())
            return empty;
        assert(a.itemIndex != (uint32)-1);
        return a.g->texts[a.itemIndex].collisionGlyphsRects;
    };
    const auto &ra = getRects(a);
    const auto &rb = getRects(b);

    if (ra.empty() && rb.empty())
        return true;

    const auto &test = [](const std::vector<Rect> &a, const Rect &b)
    {
        for (const auto &ra : a)
            if (Rect::overlaps(ra, b))
                return true;
        return false;
    };

    if (ra.empty() != rb.empty())
    {
        if (ra.empty())
            return test(rb, a.collisionRect);
        else
            return test(ra, b.collisionRect);
    }

    assert(!ra.empty() && !rb.empty());
    for (const auto &ca : ra)
    {
        if (!Rect::overlaps(ca, b.collisionRect))
            continue;
        if (test(rb, ca))
            return true;
    }
    return false;
}

float Rect::width() const
{
    return b[0] - a[0];
}

float Rect::height() const
{
    return b[1] - a[1];
}

GeodataJob::GeodataJob(const std::shared_ptr<GeodataTile> &g,
    uint32 itemIndex)
    : g(g), labelOffset(0, 0), refPoint(nan2().cast<float>()),
    itemIndex(itemIndex), importance(nan1()), opacity(1), depth(nan1())
{}

vec3f GeodataJob::modelPosition() const
{
    assert(itemIndex != (uint32)-1);
    return g->points[itemIndex].modelPosition;
}

vec3 GeodataJob::worldPosition() const
{
    assert(itemIndex != (uint32)-1);
    return g->points[itemIndex].worldPosition;
}

vec3f GeodataJob::worldUp() const
{
    assert(itemIndex != (uint32)-1);
    return g->points[itemIndex].worldUp;
}

bool RenderViewImpl::geodataTestVisibility(
    const float visibility[4],
    const vec3 &pos, const vec3f &up)
{
    vec3 eye = rawToVec3(draws->camera.eye);
    double distance = length(vec3(eye - pos));
    if (!std::isnan(visibility[0]) && distance > visibility[0])
        return false;
    distance *= 2 / draws->camera.proj[5];
    if (!std::isnan(visibility[1]) && distance < visibility[1])
        return false;
    if (!std::isnan(visibility[2]) && distance > visibility[2])
        return false;
    if (!std::isnan(visibility[3])
        && dot(normalize(vec3(eye - pos)).cast<float>(), up)
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
        if (std::isnan(t))
            return nan1();
        vec3 p = o + d * t;
        p = vec3(p[0], p[1], p[2] / r); // sphere to ellipsoid
        return length(vec3(p - o));
    }
}

bool RenderViewImpl::geodataDepthVisibility(const vec3 &pos, float threshold)
{
    if (std::isnan(threshold))
        return true;

    vec3 diff = vec3(rawToVec3(draws->camera.eye) - pos);
    vec3 dir = normalize(diff);

    // if the feature is very far, the depth buffer is not precise enough
    // we compare it with ellipsoid instead
    if (diff.squaredNorm() > 1e13)
    {
        double de = rayEllipsoidTest(rawToVec3(draws->camera.eye),
            -dir, body->majorRadius, body->minorRadius);
        if (std::isnan(de))
            return true;
        double df = length(diff);
        return df < de + threshold;
    }

    // compare to the depth buffer
    vec3 p3 = pos + dir * threshold;
    vec4 p4 = depthBuffer.getConv() * vec3to4(p3, 1);
    p3 = vec4to3(p4, true);
    double d = depthBuffer.value(p3[0], p3[1]) * 2 - 1;
    if (!std::isnan(d))
        return p3[2] < d;

    // if the depth value is invalid (eg. sky), the feature is visible
    return true;
}

mat4 RenderViewImpl::depthOffsetCorrection(
    const std::shared_ptr<GeodataTile> &g) const
{
    vec3 zbo = rawToVec3(g->spec.commonData.zBufferOffset).cast<double>();
    double off = dot(zbo, zBufferOffsetValues) * 0.0001;
    double off2 = (off + 1) * 2 - 1;
    mat4 s = scaleMatrix(vec3(1, 1, off2));
    return davidProjInv * s * davidProj;
}

namespace
{

struct mat3x4f
{
    // no eigen matrix!!!!!!
    // one does not have time to mess with the indices
    float data[12];
};

mat3x4f rectTransform(const GeodataJob &job, const Rect &rect)
{
    vec2f c = (rect.a + rect.b) * 0.5 - job.refPoint;
    vec2f s = (rect.b - rect.a) * 0.5;
    mat3x4f r;
    for (int i = 0; i < 12; i++)
        r.data[i] = 0;
    r.data[0] = s[0];
    r.data[5] = s[1];
    r.data[8] = c[0];
    r.data[9] = c[1];
    r.data[10] = 1;
    return r;
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

vec4f fontOutline(float size, const vec4f &specOutline)
{
    float os = std::sqrt(2) / size;
    return specOutline.cwiseProduct(vec4f(1, 1, os, os));
}

} // namespace

void RenderViewImpl::renderGeodataQuad(const GeodataJob &job,
    const Rect &rect, const vec4f &color)
{
    assert(lastUboViewPointer == job.g.get());
    if (!rect.valid())
        return;

    struct uboColorData
    {
        mat3x4f screen;
        vec4f modelPos;
        vec4f color;
    } data;

    data.screen = rectTransform(job, rect);
    data.modelPos = vec3to4(job.modelPosition(), 1.f);
    data.color = color;

    auto ubo = getUbo();
    ubo->debugId = "UboColor";
    ubo->bind();
    ubo->load(data);
    ubo->bindToIndex(2);

    context->shaderGeodataColor->bind();
    context->meshQuad->bind();
    context->meshQuad->dispatch();
}

void RenderViewImpl::bindUboView(const std::shared_ptr<GeodataTile> &g)
{
    if (g.get() == lastUboViewPointer)
        return;
    lastUboViewPointer = g.get();

    mat4 model = rawToMat4(g->spec.model);
    mat4 mv = depthOffsetCorrection(g) * view * model;
    mat4 mvp = proj * mv;
    mat4 mvInv = mv.inverse();
    mat4 mvpInv = mvp.inverse();

    // view ubo
    struct UboViewData
    {
        mat4f mvp;
        mat4f mvpInv;
        mat4f mv;
        mat4f mvInv;
    } data;

    data.mvp = mvp.cast<float>();
    data.mvpInv = mvpInv.cast<float>();
    data.mv = mv.cast<float>();
    data.mvInv = mvInv.cast<float>();

    auto ubo = getUbo();
    ubo->debugId = "UboViewData";
    ubo->bind();
    ubo->load(data);
    ubo->bindToIndex(1);
}

void RenderViewImpl::renderGeodata()
{
    OPTICK_EVENT();

    glDepthMask(GL_FALSE);

    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    computeZBufferOffsetValues();
    bindUboCamera();
    generateJobs();
    sortJobsByZIndexAndImportance();
    if (options.renderGeodataDebug == 1)
        renderJobsDebugImportance();
    filterJobsByResolvingCollisions();
    processJobsHysteresis();
    sortJobsByZIndexAndDepth();
    renderJobs();
    if (options.renderGeodataDebug == 2)
        renderJobsDebugRects();
    if (options.renderGeodataDebug == 3)
        renderJobsDebugGlyphs();
    geodataJobs.clear();

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
    } data;

    data.proj = proj.cast<float>();
    data.cameraParams = vec4f(width, height,
        draws->camera.viewExtent, 0);

    uboGeodataCamera->bind();
    uboGeodataCamera->load(data);
    uboGeodataCamera->bindToIndex(0);
}

void RenderViewImpl::regenerateJobCommon(GeodataJob &j)
{
    const auto &g = j.g;

    // importance
    if (!g->spec.importances.empty())
    {
        j.importance = j.itemIndex == (uint32)-1
            ? g->spec.importances[0]
            : g->spec.importances[j.itemIndex];
        j.importance
            -= g->spec.commonData.importanceDistanceFactor
            * std::log(length(vec3(j.worldPosition()
                - rawToVec3(draws->camera.eye))));
    }

    // refPoint and depth
    const vec4 clip = viewProj * vec3to4(j.worldPosition(), 1);
    const vec3 ndc = vec4to3(clip, true);
    j.refPoint = vec3to2(ndc).cast<float>();
    j.depth = ndc[2];
}

void RenderViewImpl::regenerateJobIcon(GeodataJob &j)
{
    const auto &g = j.g;
    const auto &c = g->spec.commonData.icon;
    if (c.scale > 0)
    {
        const auto &s = g->spec.iconCoords[j.itemIndex];
        float sc = c.scale * 2; // NDC space is twice as large
        vec2f ss = sc * vec2f(s[4] / width, s[5] / height);
        j.iconRect.a = j.iconRect.b = j.refPoint + vec2f(
            g->spec.commonData.icon.offset[0] * 2 / width,
            g->spec.commonData.icon.offset[1] * 2 / height
        ) - numericOrigin(g->spec.commonData.icon.origin)
            .cwiseProduct(ss);
        j.iconRect.b += ss;
    }
}

void RenderViewImpl::regenerateJobStick(GeodataJob &j)
{
    const auto &g = j.g;
    const auto &s = g->spec.commonData.stick;
    if (s.width > 0)
    {
        float stick = s.heightMax;
        {
            vec3f toEye = normalize(rawToVec3(
                draws->camera.eye)).cast<float>();
            vec3f toJob = normalize(vec3(j.worldPosition()
                - rawToVec3(draws->camera.eye))).cast<float>();
            vec3f camUp = vec4to3(vec4(viewInv
                * vec4(0, 1, 0, 0)), false).cast<float>();
            float localTilt = std::max(1 - dot(-toJob, j.worldUp()), 0.f);
            float cameraTilt = std::max(dot(toEye, camUp), 0.f);
            stick *= localTilt * cameraTilt;
        }
        if (stick < s.heightThreshold)
            stick = 0;
        stick *= 2; // NDC space is twice as large
        j.stickRect.a = j.stickRect.b = j.refPoint;
        float w = s.width / width;
        float h = stick / height;
        j.stickRect.a[0] -= w;
        j.stickRect.b[0] += w;
        j.stickRect.b[1] += h;

        // offset others
        float ro = (stick > 1e-7 ? stick + s.offset : 0) / height;
        j.labelRect.a[1] += ro;
        j.labelRect.b[1] += ro;
        j.labelOffset[1] += ro;
        j.iconRect.a[1] += ro;
        j.iconRect.b[1] += ro;
    }
}

void RenderViewImpl::regenerateJobCollision(GeodataJob &j)
{
    const auto &g = j.g;
    if (j.g->spec.commonData.preventOverlap)
    {
        vec2f marginLabel = rawToVec2(g->spec.unionData.labelScreen.margin)
            .cwiseProduct(vec2f(2.f / width, 2.f / height));
        Rect label = j.labelRect;
        label.a -= marginLabel;
        label.b += marginLabel;

        vec2f marginIcon = rawToVec2(g->spec.commonData.icon.margin)
            .cwiseProduct(vec2f(2.f / width, 2.f / height));
        Rect icon = j.iconRect;
        icon.a -= marginIcon;
        icon.b += marginIcon;

        j.collisionRect = Rect::merge(label, icon);
    }
}

bool RenderViewImpl::regenerateJobLabelFlat(GeodataJob &j)
{
    bool valid = vts::renderer::regenerateJobLabelFlat(this, j);
    const auto &g = j.g;
    const auto &t = g->texts[j.itemIndex];
    j.collisionRect = Rect();
    for (const auto &it : t.collisionGlyphsRects)
        j.collisionRect = Rect::merge(j.collisionRect, it);
    return valid;
}

void RenderViewImpl::regenerateJobLabelScreen(GeodataJob &j)
{
    const auto &g = j.g;
    const auto &t = g->texts[j.itemIndex];
    float sc = options.textScale * 2; // NDC space is twice as large
    vec2f sd = vec2f(sc / width, sc / height);
    vec2f org = numericOrigin(g->spec.unionData.labelScreen.origin)
        - vec2f(0.5f, 0.5f);
    j.labelOffset = vec2f(
        g->spec.unionData.labelScreen.offset[0] * 2 / width,
        g->spec.unionData.labelScreen.offset[1] * 2 / height
    ) - org.cwiseProduct(t.originSize).cwiseProduct(sd);
    vec2f off = j.labelOffset + j.refPoint;
    j.labelRect = Rect(
        t.collision.a.cwiseProduct(sd) + off,
        t.collision.b.cwiseProduct(sd) + off
    );
}

bool RenderViewImpl::regenerateJob(GeodataJob &j)
{
    switch (j.g->spec.type)
    {
    case GpuGeodataSpec::Type::PointFlat:
    case GpuGeodataSpec::Type::PointScreen:
    case GpuGeodataSpec::Type::LineFlat:
    case GpuGeodataSpec::Type::LineScreen:
    case GpuGeodataSpec::Type::Triangles:
        break; // nothing

    case GpuGeodataSpec::Type::IconFlat:
    {
        // todo
    } break;

    case GpuGeodataSpec::Type::IconScreen:
    {
        regenerateJobCommon(j);
        regenerateJobIcon(j);
        regenerateJobStick(j);
        regenerateJobCollision(j);
    } break;

    case GpuGeodataSpec::Type::LabelFlat:
    {
        regenerateJobCommon(j);
        return regenerateJobLabelFlat(j);
    } break;

    case GpuGeodataSpec::Type::LabelScreen:
    {
        regenerateJobCommon(j);
        regenerateJobLabelScreen(j);
        regenerateJobIcon(j);
        regenerateJobStick(j);
        regenerateJobCollision(j);
    } break;

    case GpuGeodataSpec::Type::Invalid:
        throw std::invalid_argument("Invalid geodata type enum");
    }
    return true;
}

void RenderViewImpl::generateJobs()
{
    geodataJobs.clear();
    for (const auto &t : draws->geodata)
    {
        std::shared_ptr<GeodataTile> g
            = std::static_pointer_cast<GeodataTile>(t.geodata);

        if (draws->camera.viewExtent
            < g->spec.commonData.tileVisibility[0]
            || draws->camera.viewExtent
            >= g->spec.commonData.tileVisibility[1])
            continue;

        switch (g->spec.type)
        {
        case GpuGeodataSpec::Type::Invalid:
            throw std::invalid_argument("Invalid geodata type enum");

        case GpuGeodataSpec::Type::PointFlat:
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::LineFlat:
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::Triangles:
        {
            // one job for entire tile
            geodataJobs.emplace_back(g, uint32(-1));
        } break;

        case GpuGeodataSpec::Type::IconFlat:
        case GpuGeodataSpec::Type::LabelFlat:
        case GpuGeodataSpec::Type::IconScreen:
        case GpuGeodataSpec::Type::LabelScreen:
        {
            if (!g->checkTextures())
                continue;

            // individual jobs for each icon/label
            for (uint32 index = 0, indexEnd = g->points.size();
                index < indexEnd; index++)
            {
                GeodataJob j(g, index);

                if (!geodataTestVisibility(
                    g->spec.commonData.visibilities,
                    j.worldPosition(), j.worldUp()))
                    continue;

                if (!geodataDepthVisibility(j.worldPosition(),
                    g->spec.commonData.depthVisibilityThreshold))
                    continue;

                if (regenerateJob(j))
                    geodataJobs.push_back(std::move(j));
            }
        } break;
        }
    }
}

void RenderViewImpl::sortJobsByZIndexAndImportance()
{
    std::sort(geodataJobs.begin(), geodataJobs.end(),
        [](const GeodataJob &a, const GeodataJob &b)
        {
            auto az = a.g->spec.commonData.zIndex;
            auto bz = b.g->spec.commonData.zIndex;
            if (az == bz)
            {
                if (std::isnan(a.importance) || std::isnan(b.importance))
                    return a.g->spec.type < b.g->spec.type;
                return a.importance > b.importance;
            }
            return az < bz;
        });
}

void RenderViewImpl::renderJobsDebugRects()
{
    static const vec4f colorCollision = vec4f(0.5, 0.5, 0.5, 0.4);
    static const vec4f colorStick = vec4f(0, 0, 0.5, 0.4);
    static const vec4f colorIcon = vec4f(0, 0.5, 0, 0.4);
    static const vec4f colorLabel = vec4f(0.5, 0, 0, 0.4);
    static const vec4f colorRef = vec4f(1, 0, 1, 1);
    for (const GeodataJob &job : geodataJobs)
    {
        if (job.itemIndex == (uint32)-1)
            continue;
        bindUboView(job.g);
        renderGeodataQuad(job, job.collisionRect, colorCollision);
        renderGeodataQuad(job, job.stickRect, colorStick);
        renderGeodataQuad(job, job.iconRect, colorIcon);
        renderGeodataQuad(job, job.labelRect, colorLabel);
        renderGeodataQuad(job, pointToRect(job.refPoint,
            width, height), colorRef);
    }
}

void RenderViewImpl::renderJobsDebugGlyphs()
{
    static const vec4f colorCollision = vec4f(1, 1, 0, 0.4);
    for (const GeodataJob &job : geodataJobs)
    {
        if (job.itemIndex == (uint32)-1)
            continue;
        bindUboView(job.g);
        for (const auto &it : job.g->texts[job.itemIndex].collisionGlyphsRects)
            renderGeodataQuad(job, it, colorCollision);
    }
}

void RenderViewImpl::renderJobsDebugImportance()
{
    uint32 cnt = geodataJobs.size();
    struct Quad
    {
        const GeodataJob *job;
        vec4f color;
        Quad(const GeodataJob *job, vec4f color) : job(job), color(color)
        {}
    };
    std::vector<Quad> quads;
    quads.reserve(cnt);
    uint32 i = 0;
    for (const GeodataJob &job : geodataJobs)
    {
        if (job.itemIndex == (uint32)-1)
            continue;
        vec3f color = convertToRainbowColor(1 - float(i++) / cnt);
        quads.emplace_back(&job, vec3to4(color, 0.35f));
    }
    std::sort(quads.begin(), quads.end(),
        [](const Quad &a, const Quad &b)
        {
            return a.job->depth > b.job->depth;
        });
    for (const Quad &a : quads)
    {
        bindUboView(a.job->g);
        renderGeodataQuad(*a.job, a.job->collisionRect, a.color);
    }
}

void RenderViewImpl::filterJobsByResolvingCollisions()
{
    const float pixels = width * height;
    uint32 index = 0;
    std::vector<GeodataJob> result;
    result.reserve(geodataJobs.size());
    for (auto &it : geodataJobs)
    {
        const float limitFactor = it.g->spec.commonData
            .featuresLimitPerPixelSquared;
        if (index > limitFactor * pixels)
            continue;
        if (it.collisionRect.valid())
        {
            bool ok = true;
            for (const auto &r : result)
            {
                if (!r.collisionRect.valid())
                    continue;
                if (overlaps(it, r))
                {
                    ok = false;
                    break;
                }
            }
            if (!ok)
                continue;
        }
        if (!std::isnan(limitFactor))
            index++;
        result.push_back(std::move(it));
    }
    std::swap(result, geodataJobs);
}

void RenderViewImpl::processJobsHysteresis()
{
    if (!options.geodataHysteresis)
    {
        hysteresisJobs.clear();
        return;
    }

    for (auto &it = hysteresisJobs.begin(); it != hysteresisJobs.end(); )
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
            geodataJobs.push_back(std::move(it.second));
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
            {
                if (std::isnan(a.depth) || std::isnan(b.depth))
                    return a.g->spec.type < b.g->spec.type;
                return a.depth > b.depth;
            }
            return az < bz;
        });
}

void RenderViewImpl::renderStick(const GeodataJob &job)
{
    const auto &s = job.g->spec.commonData.stick;
    vec4f color = rawToVec4(s.color);
    color[3] *= job.opacity;
    renderGeodataQuad(job, job.stickRect, color);
}

void RenderViewImpl::renderPointOrLine(const GeodataJob &job)
{
    const auto &g = job.g;
    assert(job.itemIndex == (uint32)-1);
    bindUboView(g);
    g->uniform->bindToIndex(2);
    g->texture->bind();
    Mesh *msh = g->mesh.get();
    msh->bind();
    glEnable(GL_STENCIL_TEST);
    msh->dispatch();
    glDisable(GL_STENCIL_TEST);
}

void RenderViewImpl::renderIcon(const GeodataJob &job)
{
    struct UboIcon
    {
        mat3x4f screen;
        vec4f modelPos;
        vec4f color;
        vec4f uvs;
    } data;

    const auto &icon = job.g->spec.commonData.icon;

    data.screen = rectTransform(job, job.iconRect);
    data.modelPos = vec3to4(job.modelPosition(), 1.f);
    data.color = rawToVec4(icon.color);
    data.color[3] *= job.opacity;
    data.uvs = rawToVec4(job.g->spec.iconCoords[job.itemIndex].data());

    auto ubo = getUbo();
    ubo->debugId = "UboIcon";
    ubo->bind();
    ubo->load(data);
    ubo->bindToIndex(2);

    ((Texture*)job.g->spec.bitmap.get())->bind();

    context->shaderGeodataIconScreen->bind();
    context->meshQuad->bind();
    context->meshQuad->dispatch();
}

void RenderViewImpl::renderLabelFlat(const GeodataJob &job)
{
    struct UboLabelFlat
    {
        vec4f color[2];
        vec4f outline;
        vec4f position; // xyz
        vec4f coordinates[1000];
    } data;

    const auto &g = job.g;
    const auto &t = g->texts[job.itemIndex];

    // generate vertex positions
    float scale;
    {
        std::vector<vec3> worldPos;
        preDrawJobLabelFlat(this, job, worldPos, scale);
        assert(worldPos.size() >= 2);
        assert(worldPos.size() <= 125);
        std::vector<vec3> worldFwd;
        worldFwd.reserve(worldPos.size());
        worldFwd.push_back(worldPos[1] - worldPos[0]);
        for (uint32 i = 1, e = worldPos.size() - 1; i != e; i++)
            worldFwd.push_back(worldPos[i + 1] - worldPos[i - 1]);
        worldFwd.push_back(worldPos[worldPos.size() - 1]
            - worldPos[worldPos.size() - 2]);
        assert(worldPos.size() == worldFwd.size());
        for (auto &it : worldFwd)
            it = normalize(it);
        mat4 vp = proj * depthOffsetCorrection(g) * view;
        //vec3 right = vec4to3(vec4(viewInv * vec4(1, 0, 0, 0)));
        //vec3 up = vec4to3(vec4(viewInv * vec4(0, 1, 0, 0)));
        vec4f *c = data.coordinates;
        for (uint32 i = 0, e = worldPos.size(); i != e; i++)
        {
            vec3 center = worldPos[i];
            vec3 right = worldFwd[i];
            vec3 up = normalize(cross(normalize(center), right));
            for (uint32 j = 0; j < 4; j++)
            {
                vec4f coord = t.coordinates[i * 4 + j];
                vec3 wp = center + (coord[0] * right
                    + coord[1] * up) * scale;
                *c++ = vec4(vp * vec3to4(wp, 1)).cast<float>();
                *c++ = coord;
            }
        }
    }

    data.color[0] = rawToVec4(g->spec.unionData.labelFlat.color2);
    data.color[1] = rawToVec4(g->spec.unionData.labelFlat.color);
    data.color[0][3] *= job.opacity;
    data.color[1][3] *= job.opacity;
    data.outline = fontOutline(g->spec.unionData.labelFlat.size,
        rawToVec4(g->spec.unionData.labelFlat.outline));
    data.position = vec3to4(job.modelPosition(), 0);
    assert(t.coordinates.size() <= 500);

    context->shaderGeodataLabelFlat->bind();
    auto ubo = getUbo();
    ubo->debugId = "UboLabelFlat";
    ubo->bind();
    ubo->load(&data,
        16 * sizeof(float) + 4 * sizeof(float) * t.coordinates.size() * 2);
    ubo->bindToIndex(2);

    context->meshEmpty->bind();
    for (int pass = 0; pass < 2; pass++)
    {
        context->shaderGeodataLabelFlat->uniform(0, pass);
        for (auto &w : t.subtexts)
        {
            w.texture->bind();
            context->meshEmpty->dispatch(
                w.indicesStart, w.indicesCount);
        }
    }
}

void RenderViewImpl::renderLabelScreen(const GeodataJob &job)
{
    struct UboLabelScreen
    {
        vec4f color[2];
        vec4f outline;
        vec4f position; // xyz, scale
        vec4f offset;
        vec4f coordinates[500];
    } data;

    const auto &g = job.g;
    const auto &t = g->texts[job.itemIndex];

    data.color[0] = rawToVec4(g->spec.unionData.labelScreen.color2);
    data.color[1] = rawToVec4(g->spec.unionData.labelScreen.color);
    data.color[0][3] *= job.opacity;
    data.color[1][3] *= job.opacity;
    data.outline = fontOutline(g->spec.unionData.labelScreen.size,
        rawToVec4(g->spec.unionData.labelScreen.outline));
    data.position = vec3to4(job.modelPosition(), options.textScale * 2);
    data.offset = vec4f(job.labelOffset[0], job.labelOffset[1], 0, 0);
    assert(t.coordinates.size() <= 500);
    std::copy(t.coordinates.begin(), t.coordinates.end(), data.coordinates);

    context->shaderGeodataLabelScreen->bind();
    auto ubo = getUbo();
    ubo->debugId = "UboLabelScreen";
    ubo->bind();
    ubo->load(&data,
        20 * sizeof(float) + 4 * sizeof(float) * t.coordinates.size());
    ubo->bindToIndex(2);

    context->meshEmpty->bind();
    for (int pass = 0; pass < 2; pass++)
    {
        context->shaderGeodataLabelScreen->uniform(0, pass);
        for (auto &w : t.subtexts)
        {
            w.texture->bind();
            context->meshEmpty->dispatch(
                w.indicesStart, w.indicesCount);
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
        case GpuGeodataSpec::Type::Invalid:
            throw std::invalid_argument("Invalid geodata type enum");

        case GpuGeodataSpec::Type::PointFlat:
        {
            context->shaderGeodataPointFlat->bind();
            renderPointOrLine(job);
        } break;

        case GpuGeodataSpec::Type::PointScreen:
        {
            context->shaderGeodataPointScreen->bind();
            renderPointOrLine(job);
        } break;

        case GpuGeodataSpec::Type::LineFlat:
        {
            context->shaderGeodataLineFlat->bind();
            renderPointOrLine(job);
        } break;

        case GpuGeodataSpec::Type::LineScreen:
        {
            context->shaderGeodataLineScreen->bind();
            renderPointOrLine(job);
        } break;

        case GpuGeodataSpec::Type::IconFlat:
        {
            // todo
        } break;

        case GpuGeodataSpec::Type::IconScreen:
        {
            if (!geodataTestVisibility(
                g->spec.commonData.visibilities,
                job.worldPosition(), job.worldUp()))
                continue;

            bindUboView(g);
            renderStick(job);
            renderIcon(job);
        } break;

        case GpuGeodataSpec::Type::LabelFlat:
        {
            if (!geodataTestVisibility(
                g->spec.commonData.visibilities,
                job.worldPosition(), job.worldUp()))
                continue;

            bindUboView(g);
            renderLabelFlat(job);
        } break;

        case GpuGeodataSpec::Type::LabelScreen:
        {
            if (!geodataTestVisibility(
                g->spec.commonData.visibilities,
                job.worldPosition(), job.worldUp()))
                continue;

            bindUboView(g);
            renderStick(job);
            if (job.g->spec.commonData.icon.scale > 0)
                renderIcon(job);
            renderLabelScreen(job);
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
        }
    }
    lastUboViewPointer = nullptr;
}

void RenderContext::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec,
    const std::string &debugId)
{
    auto r = std::make_shared<GeodataTile>();
    r->load(&*impl, info, spec, debugId);
    info.userData = r;
}

} } // namespace vts renderer

