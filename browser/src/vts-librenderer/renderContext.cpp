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

namespace vts { namespace renderer
{

void ShaderAtm::initializeAtmosphere()
{
    bindUniformBlockLocations({
            { "uboAtm", 0 }
        });
    bindTextureLocations({
            { "texAtmDensity", 4 }
        });
}

RenderContextImpl::RenderContextImpl(RenderContext *api) : api(api)
{
    std::string atm = readInternalMemoryBuffer(
        "data/shaders/atmosphere.inc.glsl").str();
    std::string geo = readInternalMemoryBuffer(
        "data/shaders/geodata.inc.glsl").str();

    // load texture compas
    {
        texCompas = std::make_shared<Texture>();
        vts::GpuTextureSpec spec(vts::readInternalMemoryBuffer(
            "data/textures/compas.png"));
        spec.verticalFlip();
        vts::ResourceInfo ri;
        texCompas->load(ri, spec, "data/textures/compas.png");
    }

    // load shader texture
    {
        shaderTexture = std::make_shared<Shader>();
        shaderTexture->debugId
            = "data/shaders/texture.*.glsl";
        shaderTexture->loadInternal(
            "data/shaders/texture.vert.glsl",
            "data/shaders/texture.frag.glsl");
        shaderTexture->loadUniformLocations({
                "uniMvp",
                "uniUvm"
            });
        shaderTexture->bindTextureLocations({
                { "uniTexture", 0 }
            });
    }

    // load shader surface
    {
        shaderSurface = std::make_shared<ShaderAtm>();
        shaderSurface->debugId
            = "data/shaders/surface.*.glsl";
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/surface.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/surface.frag.glsl");
        shaderSurface->load(vert.str(), atm + frag.str());
        shaderSurface->bindUniformBlockLocations({
                 { "uboSurface", 1 }
             });
        shaderSurface->bindTextureLocations({
                { "texColor", 0 },
                { "texMask", 1 }
            });
        shaderSurface->initializeAtmosphere();
    }

    // load shader infographic
    {
        shaderInfographics = std::make_shared<Shader>();
        shaderInfographics->debugId
            = "data/shaders/infographic.*.glsl";
        shaderInfographics->loadInternal(
            "data/shaders/infographics.vert.glsl",
            "data/shaders/infographics.frag.glsl");
        shaderInfographics->bindUniformBlockLocations({
                 { "uboInfographics", 1 }
             });
        shaderInfographics->bindTextureLocations({
                { "texColor", 0 },
                { "texDepth", 6 }
            });
    }

    // load shader background
    {
        shaderBackground = std::make_shared<ShaderAtm>();
        shaderBackground->debugId
            = "data/shaders/background.*.glsl";
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/background.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/background.frag.glsl");
        shaderBackground->load(vert.str(), atm + frag.str());
        shaderBackground->loadUniformLocations({
                "uniCorners[0]",
                "uniCorners[1]",
                "uniCorners[2]",
                "uniCorners[3]"
            });
        shaderBackground->initializeAtmosphere();
    }

    // load shader copy depth
    {
        shaderCopyDepth = std::make_shared<Shader>();
        shaderCopyDepth->debugId
            = "data/shaders/copyDepth.*.glsl";
        shaderCopyDepth->loadInternal(
            "data/shaders/copyDepth.vert.glsl",
            "data/shaders/copyDepth.frag.glsl");
        shaderCopyDepth->loadUniformLocations({
                "uniTexPos"
            });
        shaderCopyDepth->bindTextureLocations({
                { "texDepth", 0 }
            });
    }

    // load mesh quad
    {
        meshQuad = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
            "data/meshes/quad.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo ri;
        meshQuad->load(ri, spec, "data/meshes/quad.obj");
    }

    // load mesh rect
    {
        meshRect = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
            "data/meshes/rect.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Triangles);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo ri;
        meshRect->load(ri, spec, "data/meshes/rect.obj");
    }

    // load mesh line
    {
        meshLine = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec(vts::readInternalMemoryBuffer(
            "data/meshes/line.obj"));
        assert(spec.faceMode == vts::GpuMeshSpec::FaceMode::Lines);
        spec.attributes[0].enable = true;
        spec.attributes[0].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[0].components = 3;
        spec.attributes[1].enable = true;
        spec.attributes[1].stride = sizeof(vts::vec3f) + sizeof(vts::vec2f);
        spec.attributes[1].components = 2;
        spec.attributes[1].offset = sizeof(vts::vec3f);
        vts::ResourceInfo ri;
        meshLine->load(ri, spec, "data/meshes/line.obj");
    }

    // load mesh empty
    {
        meshEmpty = std::make_shared<Mesh>();
        vts::GpuMeshSpec spec;
        spec.faceMode = vts::GpuMeshSpec::FaceMode::Triangles;
        vts::ResourceInfo ri;
        meshEmpty->load(ri, spec, "meshEmpty");
    }

    // load shader geodata color
    {
        shaderGeodataColor = std::make_shared<Shader>();
        shaderGeodataColor->debugId
            = "data/shaders/geodataColor.*.glsl";
        shaderGeodataColor->loadInternal(
            "data/shaders/geodataColor.vert.glsl",
            "data/shaders/geodataColor.frag.glsl"
        );
        shaderGeodataColor->loadUniformLocations({
                "uniMvp",
                "uniColor"
            });
    }

    // load shader geodata line
    {
        shaderGeodataLine = std::make_shared<Shader>();
        shaderGeodataLine->debugId
            = "data/shaders/geodataLine.*.glsl";
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataLine.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataLine.frag.glsl");
        shaderGeodataLine->load(geo + vert.str(), geo + frag.str());
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
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataPoint.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataPoint.frag.glsl");
        shaderGeodataPoint->load(geo + vert.str(), geo + frag.str());
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
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataPointLabel.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataPointLabel.frag.glsl");
        shaderGeodataPointLabel->load(geo + vert.str(), geo + frag.str());
        shaderGeodataPointLabel->loadUniformLocations({
                "uniPass"
            });
        shaderGeodataPointLabel->bindTextureLocations({
                { "texGlyphs", 0 }
            });
        shaderGeodataPointLabel->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboText", 2 }
            });
    }

    // load shader geodata triangle
    {
        shaderGeodataTriangle = std::make_shared<Shader>();
        shaderGeodataTriangle->debugId
            = "data/shaders/geodataTriangle.*.glsl";
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataTriangle.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataTriangle.frag.glsl");
        shaderGeodataTriangle->load(geo + vert.str(), geo + frag.str());
        shaderGeodataTriangle->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboTriangleData", 2 }
            });
    }

    CHECK_GL("initialize");
}

RenderContextImpl::~RenderContextImpl()
{}

} } // namespace vts renderer

