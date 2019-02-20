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

#include <map>

#include "renderer.hpp"
#include "font.hpp"

#include <vts-browser/resources.hpp>
#include <vts-browser/cameraDraws.hpp>

namespace vts { namespace renderer
{

namespace priv
{

class GeodataBase
{
public:
    GeodataBase() : renderer(nullptr), info(nullptr)
    {}

    GpuGeodataSpec spec;
    RendererImpl *renderer;
    ResourceInfo *info;
    mat4 model;
    mat4 modelInv;

    virtual void load(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp) = 0;

    void loadInit(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp)
    {
        this->spec = std::move(specp);
        this->info = &info;
        this->renderer = renderer;

        model = rawToMat4(spec.model);
        modelInv = model.inverse();
    }

    void loadFinish()
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

    void addMemory(ResourceInfo &other)
    {
        info->ramMemoryCost += other.ramMemoryCost;
        info->gpuMemoryCost += other.gpuMemoryCost;
    }

    vec3f localUp(const vec3f &p) const
    {
        vec3 dl = p.cast<double>();
        vec3 dw = vec4to3(vec4(model * vec3to4(dl, 1)));
        vec3 upw = normalize(dw);
        vec3 tw = dw + upw;
        vec3 tl = vec4to3(vec4(modelInv * vec3to4(tw, 1)));
        return (tl - dl).cast<float>();
    }

    uint32 getTotalPoints() const
    {
        uint32 totalPoints = 0;
        uint32 linesCount = spec.positions.size();
        for (uint32 li = 0; li < linesCount; li++)
            totalPoints += spec.positions[li].size();
        return totalPoints;
    }
};

class GeodataGeometry : public GeodataBase
{
public:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<UniformBuffer> uniform;

    void load(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp) override
    {
        loadInit(renderer, info, specp);

        switch (spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineFlat:
            loadLine();
            break;
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointFlat:
            loadPoint();
            break;
        case GpuGeodataSpec::Type::Triangles:
            // todo
            break;
        default:
            throw std::invalid_argument("invalid geodata type");
        }

        loadFinish();
    }

    void prepareTextureForLinesAndPoints(Buffer &&texBuffer,
        uint32 totalPoints)
    {
        GpuTextureSpec tex;
        tex.buffer = std::move(texBuffer);
        tex.width = totalPoints;
        tex.height = 2;
        tex.components = 3;
        tex.internalFormat = GL_RGB32F;
        tex.type = GpuTypeEnum::Float;
        tex.filterMode = GpuTextureSpec::FilterMode::Nearest;
        tex.wrapMode = GpuTextureSpec::WrapMode::ClampToEdge;
        ResourceInfo ri;
        renderer->rendererApi->loadTexture(ri, tex);
        this->texture = std::static_pointer_cast<Texture>(ri.userData);
        addMemory(ri);
    }

    void prepareMeshForLinesAndPoints(Buffer &&indBuffer,
        uint32 indicesCount)
    {
        GpuMeshSpec msh;
        msh.faceMode = GpuMeshSpec::FaceMode::Triangles;
        msh.indices = std::move(indBuffer);
        msh.indicesCount = indicesCount;
        ResourceInfo ri;
        renderer->rendererApi->loadMesh(ri, msh);
        this->mesh = std::static_pointer_cast<Mesh>(ri.userData);
        addMemory(ri);
    }

    void loadLine()
    {
        uint32 totalPoints = getTotalPoints(); // example: 7
        uint32 linesCount = spec.positions.size(); // 2
        uint32 segmentsCount = totalPoints - linesCount; // 5
        uint32 jointsCount = segmentsCount - linesCount; // 3
        uint32 trianglesCount = (segmentsCount + jointsCount) * 2; // 16
        uint32 indicesCount = trianglesCount * 3; // 48
        // point index = (vertex index / 4 + vertex index % 2)
        // corner = vertex index % 4

        Buffer texBuffer;
        Buffer indBuffer;

        // prepare texture buffer and mesh indices
        {
            texBuffer.resize(totalPoints * sizeof(vec3f) * 2);
            vec3f *bufPos = (vec3f*)texBuffer.data();
            vec3f *bufUps = (vec3f*)texBuffer.data() + totalPoints;
            vec3f *texBufHalf = bufUps;
            (void)texBufHalf;

            indBuffer.resize(indicesCount * sizeof(uint16));
            uint16 *bufInd = (uint16*)indBuffer.data();
            uint16 current = 0;

            for (uint32 li = 0; li < linesCount; li++)
            {
                const std::vector<std::array<float, 3>> &points
                    = spec.positions[li];
                uint32 pointsCount = points.size();
                for (uint32 pi = 0; pi < pointsCount; pi++)
                {
                    vec3f p = rawToVec3(points[pi].data());
                    vec3f u = localUp(p);
                    *bufPos++ = p;
                    *bufUps++ = u;
                    if (pi > 1)
                    { // add joint
                        *bufInd++ = current + 1 - 4;
                        *bufInd++ = current + 4 - 4;
                        *bufInd++ = current + 6 - 4;
                        *bufInd++ = current + 1 - 4;
                        *bufInd++ = current + 6 - 4;
                        *bufInd++ = current + 3 - 4;
                    }
                    if (pi > 0)
                    { // add segment
                        *bufInd++ = current + 0;
                        *bufInd++ = current + 1;
                        *bufInd++ = current + 3;
                        *bufInd++ = current + 0;
                        *bufInd++ = current + 3;
                        *bufInd++ = current + 2;
                        current += 4;
                    }
                }
                current += 4; // make a gap
            }

            assert(bufPos == texBufHalf);
            assert(bufUps == (vec3f*)texBuffer.dataEnd());
            assert(bufInd == (uint16*)indBuffer.dataEnd());
        }

        // prepare the texture
        prepareTextureForLinesAndPoints(std::move(texBuffer), totalPoints);

        // prepare the mesh
        prepareMeshForLinesAndPoints(std::move(indBuffer), indicesCount);

        // prepare UBO
        {
            struct UboLineData
            {
                vec4f color;
                vec4f visibilityRelative;
                vec4f visibilityAbsolutePlusVisibilityPlusCulling;
                vec4f typePlusUnitsPlusWidth;
            };
            UboLineData uboLineData;

            uboLineData.color = rawToVec4(spec.unionData.line.color);
            uboLineData.visibilityRelative
                = rawToVec4(spec.commonData.visibilityRelative);
            for (int i = 0; i < 2; i++)
                uboLineData.visibilityAbsolutePlusVisibilityPlusCulling[i]
                = spec.commonData.visibilityAbsolute[i];
            uboLineData.visibilityAbsolutePlusVisibilityPlusCulling[2]
                = spec.commonData.culling;
            uboLineData.visibilityAbsolutePlusVisibilityPlusCulling[3]
                = spec.commonData.visibility;
            uboLineData.typePlusUnitsPlusWidth
                = vec4f((float)spec.type, (float)spec.unionData.line.units
                    , spec.unionData.line.width, 0.f);

            uniform = std::make_shared<UniformBuffer>();
            uniform->bind();
            uniform->load(uboLineData);
            info->gpuMemoryCost += sizeof(uboLineData);
        }
    }

    void loadPoint()
    {
        uint32 totalPoints = getTotalPoints();
        uint32 trianglesCount = totalPoints * 2;
        uint32 indicesCount = trianglesCount * 3;

        Buffer texBuffer;
        Buffer indBuffer;

        // prepare texture buffer and mesh indices
        {
            texBuffer.resize(totalPoints * sizeof(vec3f) * 2);
            vec3f *bufPos = (vec3f*)texBuffer.data();
            vec3f *bufUps = (vec3f*)texBuffer.data() + totalPoints;
            vec3f *texBufHalf = bufUps;
            (void)texBufHalf;

            indBuffer.resize(indicesCount * sizeof(uint16));
            uint16 *bufInd = (uint16*)indBuffer.data();
            uint16 current = 0;

            for (uint32 li = 0, lin = spec.positions.size();
                li < lin; li++)
            {
                const std::vector<std::array<float, 3>> &points
                    = spec.positions[li];
                for (uint32 pi = 0, pin = points.size();
                    pi < pin; pi++)
                {
                    vec3f p = rawToVec3(points[pi].data());
                    vec3f u = localUp(p);
                    *bufPos++ = p;
                    *bufUps++ = u;
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 1;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 0;
                    *bufInd++ = current + 3;
                    *bufInd++ = current + 2;
                    current += 4;
                }
            }

            assert(bufPos == texBufHalf);
            assert(bufUps == (vec3f*)texBuffer.dataEnd());
            assert(bufInd == (uint16*)indBuffer.dataEnd());
        }

        // prepare the texture
        prepareTextureForLinesAndPoints(std::move(texBuffer), totalPoints);

        // prepare the mesh
        prepareMeshForLinesAndPoints(std::move(indBuffer), indicesCount);

        // prepare UBO
        {
            struct UboPointData
            {
                vec4f color;
                vec4f visibilityRelative;
                vec4f visibilityAbsolutePlusVisibilityPlusCulling;
                vec4f typePlusRadius;
            };
            UboPointData uboPointData;

            uboPointData.color = rawToVec4(spec.unionData.point.color);
            uboPointData.visibilityRelative
                = rawToVec4(spec.commonData.visibilityRelative);
            for (int i = 0; i < 2; i++)
                uboPointData.visibilityAbsolutePlusVisibilityPlusCulling[i]
                = spec.commonData.visibilityAbsolute[i];
            uboPointData.visibilityAbsolutePlusVisibilityPlusCulling[2]
                = spec.commonData.culling;
            uboPointData.visibilityAbsolutePlusVisibilityPlusCulling[3]
                = spec.commonData.visibility;
            uboPointData.typePlusRadius
                = vec4f((float)spec.type,
                (float)spec.unionData.point.radius
                    , 0.f, 0.f);

            uniform = std::make_shared<UniformBuffer>();
            uniform->bind();
            uniform->load(uboPointData);
            info->gpuMemoryCost += sizeof(uboPointData);
        }
    }
};

struct Word
{
    std::vector<vec4f> advances; // x advance, y advance, x offset, y offset
    std::vector<uint16> glyphIndices;
    std::shared_ptr<Font> font;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    uint16 fileIndex;
};

struct Text
{
    std::vector<Word> words;
    vec3f position; // model space
};

class GeodataText : public GeodataBase
{
public:
    std::vector<std::shared_ptr<Font>> fontCascade;
    std::vector<Text> texts;

    void generateWords(Text &t, const std::string &s)
    {
        // todo
        // bidi
        // split
        // harfbuzz
        // while contains tofu and has additional font available
        //   split
        //   harfbuzz

        // split the string into individual glyphs
        {
            hb_buffer_t *buffer = hb_buffer_create();

            std::shared_ptr<Font> fnt = fontCascade[0];

            hb_buffer_add_utf8(buffer, s.data(), s.length(), 0, -1);
            hb_buffer_guess_segment_properties(buffer);
            hb_shape(fnt->font, buffer, nullptr, 0);

            uint32 len = hb_buffer_get_length(buffer);
            hb_glyph_info_t *info
                = hb_buffer_get_glyph_infos(buffer, nullptr);
            hb_glyph_position_t *pos
                = hb_buffer_get_glyph_positions(buffer, nullptr);

            for (uint32 i = 0; i < len; i++)
            {
                uint16 g = info[i].codepoint;
                assert(g < fnt->glyphs.size());
                Word w;
                w.font = fnt;
                w.fileIndex = fnt->glyphs[g].fileIndex;
                w.glyphIndices.push_back(g);
                w.advances.push_back(vec4f(
                    pos[i].x_advance, pos[i].y_advance,
                    pos[i].x_offset, pos[i].y_offset
                ) / 64);
                t.words.push_back(w);
            }

            hb_buffer_destroy(buffer);
        }

        // merge consecutive words that share font and fileIndex
        {
            // todo
        }

        // global layouts & generate meshes
        {
            float xx = 0;
            for (auto &w : t.words)
            {
                static const uint32 stride = sizeof(float) * 5;
                GpuMeshSpec s;
                s.verticesCount = w.glyphIndices.size() * 4;
                s.indicesCount = w.glyphIndices.size() * 6;
                s.vertices.allocate(s.verticesCount * stride);
                s.indices.resize(s.indicesCount * sizeof(uint16));
                float *fit = (float*)s.vertices.data();
                uint16 *iit = (uint16*)s.indices.data();
                uint16 ii = 0;
                for (uint32 i = 0, e = w.advances.size(); i != e; i++)
                {
                    float px = xx + w.advances[i][2];
                    float py = w.advances[i][3];
                    float pw = 50;
                    float ph = 50; // todo
                    const auto &g = w.font->glyphs[w.glyphIndices[i]];
                    // 2--3
                    // |  |
                    // 0--1
                    *fit++ = px; *fit++ = py; *fit++ = g.uvs[0]; *fit++ = g.uvs[1]; *(sint32*)fit++ = g.plane;
                    *fit++ = px + pw; *fit++ = py; *fit++ = g.uvs[2]; *fit++ = g.uvs[1]; *(sint32*)fit++ = g.plane;
                    *fit++ = px; *fit++ = py + ph; *fit++ = g.uvs[0]; *fit++ = g.uvs[3]; *(sint32*)fit++ = g.plane;
                    *fit++ = px + pw; *fit++ = py + ph; *fit++ = g.uvs[2]; *fit++ = g.uvs[3]; *(sint32*)fit++ = g.plane;
                    // 0-1-2
                    // 1-3-2
                    *iit++ = ii + 0; *iit++ = ii + 1; *iit++ = ii + 2;
                    *iit++ = ii + 1; *iit++ = ii + 3; *iit++ = ii + 2;
                    xx += w.advances[i][0];
                    ii += 4;
                }
                assert(fit == (float*)s.vertices.dataEnd());
                assert(iit == (uint16*)s.indices.dataEnd());
                s.faceMode = GpuMeshSpec::FaceMode::Triangles;
                s.attributes[0].enable = true;
                s.attributes[0].components = 2;
                s.attributes[0].type = GpuTypeEnum::Float;
                s.attributes[0].offset = 0;
                s.attributes[0].stride = stride;
                s.attributes[1].enable = true;
                s.attributes[1].components = 2;
                s.attributes[1].type = GpuTypeEnum::Float;
                s.attributes[1].offset = sizeof(float) * 2;
                s.attributes[1].stride = stride;
                s.attributes[2].enable = true;
                s.attributes[2].components = 1;
                s.attributes[2].type = GpuTypeEnum::Int;
                s.attributes[2].offset = sizeof(float) * 4;
                s.attributes[2].stride = stride;
                w.mesh = std::make_shared<Mesh>();
                ResourceInfo inf;
                w.mesh->load(inf, s);
                addMemory(inf);
            }
        }
    }

    void load(RendererImpl *renderer, ResourceInfo &info,
        GpuGeodataSpec &specp) override
    {
        loadInit(renderer, info, specp);

        copyFonts();
        switch (spec.type)
        {
        case GpuGeodataSpec::Type::PointLabel:
            loadPointLabel();
            break;
        case GpuGeodataSpec::Type::LineLabel:
            loadLineLabel();
            break;
        default:
            throw std::invalid_argument("invalid geodata type");
        }

        loadFinish();
    }

    void copyFonts()
    {
        fontCascade.reserve(spec.fontCascade.size());
        for (auto &i : spec.fontCascade)
            fontCascade.push_back(std::static_pointer_cast<Font>(i));
    }

    void loadPointLabel()
    {
        assert(spec.texts.size() == spec.positions.size());
        for (uint32 i = 0, e = spec.texts.size(); i != e; i++)
        {
            Text t;
            generateWords(t, spec.texts[i]);
            t.position = rawToVec3(spec.positions[i][0].data());
            texts.push_back(std::move(t));
        }
    }

    void loadLineLabel()
    {
        // todo
    }

    bool checkTextures()
    {
        bool ok = true;
        for (auto &t : texts)
        {
            for (auto &w : t.words)
            {
                if (w.texture)
                    continue;
                w.texture = std::static_pointer_cast<Texture>(
                    w.font->fontHandle->requestTexture(w.fileIndex));
                ok = ok && !!w.texture;
            }
        }
        return ok;
    }
};

void RendererImpl::initializeGeodata()
{
    // load shader geodata line
    {
        shaderGeodataLine = std::make_shared<Shader>();
        shaderGeodataLine->loadInternal(
            "data/shaders/geodataLine.vert.glsl",
            "data/shaders/geodataLine.frag.glsl");
        shaderGeodataLine->loadUniformLocations({
                "uniMvp",
                "uniMvpInv",
                "uniMvInv"
            });
        shaderGeodataLine->bindTextureLocations({
                { "texLineData", 0 }
            });
        shaderGeodataLine->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboLineData", 1 }
            });
    }

    // load shader geodata point
    {
        shaderGeodataPoint = std::make_shared<Shader>();
        shaderGeodataPoint->loadInternal(
            "data/shaders/geodataPoint.vert.glsl",
            "data/shaders/geodataPoint.frag.glsl");
        shaderGeodataPoint->loadUniformLocations({
                "uniMvp",
                "uniMvpInv",
                "uniMvInv"
            });
        shaderGeodataPoint->bindTextureLocations({
                { "texPointData", 0 }
            });
        shaderGeodataPoint->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboPointData", 1 }
            });
    }

    // load shader geodata point label
    {
        shaderGeodataPointLabel = std::make_shared<Shader>();
        shaderGeodataPointLabel->loadInternal(
            "data/shaders/geodataPointLabel.vert.glsl",
            "data/shaders/geodataPointLabel.frag.glsl");
        shaderGeodataPointLabel->loadUniformLocations({
                "uniMvp",
                "uniPosition"
            });
        shaderGeodataPointLabel->bindTextureLocations({
                { "texGlyphs", 0 }
            });
        shaderGeodataPointLabel->bindUniformBlockLocations({
                { "uboCameraData", 0 }
            });
    }

    uboGeodataCamera = std::make_shared<UniformBuffer>();

    CHECK_GL("initialize geodata");
}

void initializeZBufferOffsetValues(vec3 &zBufferOffsetValues,
        mat4 &davidProj, mat4 &davidProjInv,
        CameraDraws *draws, const mat4 &viewInv, const mat4 &proj)
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

void RendererImpl::renderGeodata()
{
    //glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    // z-buffer-offset global data
    vec3 zBufferOffsetValues;
    mat4 davidProj, davidProjInv;
    initializeZBufferOffsetValues(zBufferOffsetValues,
        davidProj, davidProjInv,
        draws, viewInv, proj);

    // initialize camera data
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

        mat4 depthOffsetProj;
        {
            vec3 zbo = rawToVec3(gg->spec.commonData.zBufferOffset)
                .cast<double>();
            double off = dot(zbo, zBufferOffsetValues) * 0.0001;
            double off2 = (off + 1) * 2 - 1;
            mat4 s = scaleMatrix(vec3(1, 1, off2));
            // apply anti-david measures
            depthOffsetProj = proj * davidProjInv * s * davidProj;
        }

        mat4 model = rawToMat4(gg->spec.model);
        mat4 mvp = mat4(depthOffsetProj * view * model);

        switch (gg->spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineFlat:
        {
            GeodataGeometry *g = static_cast<GeodataGeometry*>(gg);
            shaderGeodataLine->bind();
            shaderGeodataLine->uniformMat4(0,
                mat4f(mvp.cast<float>()).data());
            shaderGeodataLine->uniformMat4(1,
                mat4f(mat4(mvp.inverse()).cast<float>()).data());
            shaderGeodataLine->uniformMat3(2,
                mat3f(mat4to3(mat4((view * model)
                    .inverse())).cast<float>()).data());
            g->uniform->bindToIndex(1);
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
            GeodataGeometry *g = static_cast<GeodataGeometry*>(gg);
            shaderGeodataPoint->bind();
            shaderGeodataPoint->uniformMat4(0,
                mat4f(mvp.cast<float>()).data());
            shaderGeodataPoint->uniformMat4(1,
                mat4f(mat4(mvp.inverse()).cast<float>()).data());
            shaderGeodataPoint->uniformMat3(2,
                mat3f(mat4to3(mat4((view * model)
                    .inverse())).cast<float>()).data());
            g->uniform->bindToIndex(1);
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
            shaderGeodataPointLabel->bind();
            shaderGeodataPointLabel->uniformMat4(0,
                mat4f(mvp.cast<float>()).data());
            for (auto &t : g->texts)
            {
                shaderGeodataPointLabel->uniformVec3(1, t.position.data());
                for (auto &w : t.words)
                {
                    w.texture->bind();
                    w.mesh->bind();
                    w.mesh->dispatch();
                    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    //w.mesh->dispatch();
                    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
            }
        } break;
        default:
        {
        } break;
        }
    }

    glDepthMask(GL_TRUE);
    //glEnable(GL_CULL_FACE);
}

} // namespace priv

void Renderer::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec)
{
    switch (spec.type)
    {
    case GpuGeodataSpec::Type::PointLabel:
    case GpuGeodataSpec::Type::LineLabel:
    {
        auto r = std::make_shared<GeodataText>();
        r->load(&*impl, info, spec);
        info.userData = r;
    } break;
    default:
    {

        auto r = std::make_shared<GeodataGeometry>();
        r->load(&*impl, info, spec);
        info.userData = r;
    } break;
    }
}

} } // namespace vts renderer

