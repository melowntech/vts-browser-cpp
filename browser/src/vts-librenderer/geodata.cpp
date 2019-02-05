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

#include "renderer.hpp"

#include <vts-browser/resources.hpp>
#include <vts-browser/cameraDraws.hpp>

namespace vts { namespace renderer
{

namespace
{

} // namespace

namespace priv
{

class Geodata
{
public:
    Geodata() : renderer(nullptr), info(nullptr)
    {}

    void load(RendererImpl *renderer, ResourceInfo &info, GpuGeodataSpec &specp)
    {
        this->spec = std::move(specp);
        this->info = &info;
        this->renderer = renderer;

        model = rawToMat4(spec.model);
        modelInv = model.inverse();

        switch (spec.type)
        {
        case GpuGeodataSpec::Type::Invalid:
            break; // do nothing
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineWorld:
            loadLine();
            break;
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointWorld:
            loadPoint();
            break;
        case GpuGeodataSpec::Type::LineLabel:
        case GpuGeodataSpec::Type::PointLabel:
        case GpuGeodataSpec::Type::Icon:
        case GpuGeodataSpec::Type::PackedPointLabelIcon:
        case GpuGeodataSpec::Type::Triangles:
            // todo
            break;
        }

        // free some memory
        std::vector<std::vector<std::array<float, 3>>>()
            .swap(spec.coordinates);
        std::vector<std::string>().swap(spec.texts);

        info.ramMemoryCost += sizeof(spec);
        this->info = nullptr;
        this->renderer = nullptr;

        CHECK_GL("load geodata");
    }

    GpuGeodataSpec spec;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<UniformBuffer> uniform;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Font> font;
    mat4 model;
    mat4 modelInv;
    RendererImpl *renderer;
    ResourceInfo *info;

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
        uint32 linesCount = spec.coordinates.size();
        for (uint32 li = 0; li < linesCount; li++)
            totalPoints += spec.coordinates[li].size();
        return totalPoints;
    }

    void loadLine()
    {
        uint32 totalPoints = getTotalPoints(); // example: 7
        uint32 linesCount = spec.coordinates.size(); // 2
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

            indBuffer.resize(indicesCount * sizeof(uint16));
            uint16 *bufInd = (uint16*)indBuffer.data();
            uint16 current = 0;

            for (uint32 li = 0; li < linesCount; li++)
            {
                const std::vector<std::array<float, 3>> &points
                    = spec.coordinates[li];
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

        // prepare the mesh
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

        // prepare UBO
        {
            struct UboLineData
            {
                vec4f color;
                vec4f visibilityRelative;
                vec4f zBufferOffsetPlusCulling;
                vec4f visibilityAbsolutePlusVisibility;
                vec4f typePlusUnitsPlusWidth;
            };
            UboLineData uboLineData;

            uboLineData.color = rawToVec4(spec.unionData.line.color);
            uboLineData.visibilityRelative
                = rawToVec4(spec.commonData.visibilityRelative);
            for (int i = 0; i < 3; i++)
                uboLineData.zBufferOffsetPlusCulling[i]
                    = spec.commonData.zBufferOffset[i];
            uboLineData.zBufferOffsetPlusCulling[3]
                = spec.commonData.culling;
            for (int i = 0; i < 2; i++)
                uboLineData.visibilityAbsolutePlusVisibility[i]
                    = spec.commonData.visibilityAbsolute[i];
            uboLineData.visibilityAbsolutePlusVisibility[3]
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
        // todo
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

    uboGeodataCamera = std::make_shared<UniformBuffer>();

    CHECK_GL("initialize geodata");
}

void RendererImpl::renderGeodata()
{
    //glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    // initialize camera data
    {
        struct UboCameraData
        {
            mat4f proj;
            vec4f cameraParams; // screen height in pixels, view extent in meters
        } ubo;

        ubo.proj = proj.cast<float>();
        ubo.cameraParams = vec4f(heightPrev, draws->camera.viewExtent, 0, 0);

        uboGeodataCamera->bind();
        uboGeodataCamera->load(ubo);
        uboGeodataCamera->bindToIndex(0);
    }

    // split geodata into zIndex arrays
    std::array<std::vector<DrawGeodataTask>, 520> zIndexArrays;
    for (const DrawGeodataTask &t : draws->geodata)
    {
        Geodata *g = (Geodata*)t.geodata.get();
        sint32 zi = g->spec.commonData.zIndex;
        assert(zi >= -256 && zi <= 256);
        zIndexArrays[zi + 256].push_back(t);
    }

    // iterate over zIndex arrays
    for (auto &tasksArray : zIndexArrays)
    {
        if (tasksArray.empty())
            continue;

        // iterate over geodata in one zIndex array
        for (const DrawGeodataTask &t : tasksArray)
        {
            Geodata *g = (Geodata*)t.geodata.get();
            switch (g->spec.type)
            {
            case GpuGeodataSpec::Type::LineScreen:
            case GpuGeodataSpec::Type::LineWorld:
            {
                shaderGeodataLine->bind();
                mat4 mvp = mat4(viewProj * rawToMat4(g->spec.model));
                shaderGeodataLine->uniformMat4(0,
                    mat4f(mvp.cast<float>()).data());
                shaderGeodataLine->uniformMat4(1,
                    mat4f(mat4(mvp.inverse()).cast<float>()).data());
                shaderGeodataLine->uniformMat3(2,
                    mat3f(mat4to3(mat4((view * rawToMat4(g->spec.model))
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
            default:
            {
            } break;
            }
        }
    }

    glDepthMask(GL_TRUE);
    //glEnable(GL_CULL_FACE);
}

} // namespace priv

void Renderer::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec)
{
    auto r = std::make_shared<Geodata>();
    r->load(&*impl, info, spec);
    info.userData = r;
}

} } // namespace vts renderer

