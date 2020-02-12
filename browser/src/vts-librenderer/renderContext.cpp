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
        GpuTextureSpec spec(vts::readInternalMemoryBuffer(
            "data/textures/compas.png"));
        spec.verticalFlip();
        ResourceInfo ri;
        texCompas->load(ri, spec, "data/textures/compas.png");
    }

    // load texture blue noise
    {
        texBlueNoise = std::make_shared<Texture>();
        Buffer buff;
        buff.allocate(64 * 64 * 16);
        for (uint32 i = 0; i < 16; i++)
        {
            std::stringstream ss;
            ss << "data/textures/blueNoise/" << i << ".png";
            GpuTextureSpec spec(vts::readInternalMemoryBuffer(ss.str()));
            assert(spec.width == 64);
            assert(spec.height == 64);
            assert(spec.components == 1);
            assert(spec.type == GpuTypeEnum::UnsignedByte);
            assert(spec.buffer.size() == 64 * 64);
            spec.verticalFlip();
            memcpy(buff.data() + (64 * 64 * i), spec.buffer.data(), 64 * 64);
        }
        glActiveTexture(GL_TEXTURE0 + 9);
        GLuint id = 0;
        glGenTextures(1, &id);
        texBlueNoise->setId(id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, id);
        texBlueNoise->setDebugId("blueNoise");
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, 64, 64, 16, 0,
            GL_RED, GL_UNSIGNED_BYTE, buff.data());
        glTexParameteri(GL_TEXTURE_2D_ARRAY,
            GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,
            GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glActiveTexture(GL_TEXTURE0 + 0);
        CHECK_GL("load texture blue noise");
    }

    // load shader texture
    {
        shaderTexture = std::make_shared<Shader>();
        shaderTexture->setDebugId(
            "data/shaders/texture.*.glsl");
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
        shaderSurface->setDebugId(
            "data/shaders/surface.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/surface.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/surface.frag.glsl");
        shaderSurface->load(atm + vert.str(), atm + frag.str());
        shaderSurface->bindUniformBlockLocations({
                 { "uboSurface", 1 }
             });
        shaderSurface->bindTextureLocations({
                { "texColor", 0 },
                { "texMask", 1 },
                { "texBlueNoise", 9 }
            });
        shaderSurface->initializeAtmosphere();
    }

    //shaderSurfaceWithNormalTexure
    {
        shaderSurfaceWithNormalTexure = std::make_shared<ShaderAtm>();
        shaderSurfaceWithNormalTexure->setDebugId(
            "data/shaders/surface.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/surface.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/surface.frag.glsl");
        shaderSurfaceWithNormalTexure->load(atm + vert.str(), "#define NORMAL_TEXTURE;\n" + atm + frag.str());
        shaderSurfaceWithNormalTexure->bindUniformBlockLocations({
                 { "uboSurface", 1 }
             });
        shaderSurfaceWithNormalTexure->bindTextureLocations({
                { "texColor", 0 },
                { "texMask", 1 },
                { "texBlueNoise", 9 }
            });
        shaderSurfaceWithNormalTexure->initializeAtmosphere();
    }
    
    // load shader infographic
    {
        shaderInfographics = std::make_shared<Shader>();
        shaderInfographics->setDebugId(
            "data/shaders/infographic.*.glsl");
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
        shaderBackground->setDebugId(
            "data/shaders/background.*.glsl");
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
        shaderCopyDepth->setDebugId(
            "data/shaders/copyDepth.*.glsl");
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

    // load shader greyscale
    {
        shaderGreyscale = std::make_shared<Shader>();
        shaderGreyscale->setDebugId(
            "data/shaders/filters/greyscale.*.glsl");
        shaderGreyscale->loadInternal(
            "data/shaders/filters/greyscale.vert.glsl",
            "data/shaders/filters/greyscale.frag.glsl");
        shaderGreyscale->bindTextureLocations({
                { "texColor", 9 }
            });
    }

    // load shader depth
    {
        shaderDepth = std::make_shared<Shader>();
        shaderDepth->setDebugId(
            "data/shaders/filters/depth.*.glsl");
        shaderDepth->loadInternal(
            "data/shaders/filters/depth.vert.glsl",
            "data/shaders/filters/depth.frag.glsl");
        shaderDepth->bindTextureLocations({
                { "texColor", 5 }
            });
    }

    // load shader normals
    {
        shaderNormal = std::make_shared<Shader>();
        shaderNormal->setDebugId(
            "data/shaders/filters/normals.*.glsl");
        shaderNormal->loadInternal(
            "data/shaders/filters/normals.vert.glsl",
            "data/shaders/filters/normals.frag.glsl");
        shaderNormal->bindTextureLocations({
                { "texNormal", 10 }
            });
    }

    // load shader dof
    {
        shaderDOF = std::make_shared<Shader>();
        shaderDOF->setDebugId(
            "data/shaders/filters/dof.*.glsl");
        shaderDOF->loadInternal(
            "data/shaders/filters/dof.vert.glsl",
            "data/shaders/filters/dof.frag.glsl");
        shaderDOF->bindTextureLocations({
                { "texColor", 9 },
                { "texDepth", 5 }
            });
    }

    // load shader dofH
    {
        shaderDOF2 = std::make_shared<Shader>();
        shaderDOF2->setDebugId(
            "data/shaders/filters/dof2.*.glsl");
        shaderDOF2->loadInternal(
            "data/shaders/filters/dof2.vert.glsl",
            "data/shaders/filters/dof2.frag.glsl");
        shaderDOF2->bindTextureLocations({
                { "texColor", 9 },
                { "texDepth", 5 }
            });
    }

    // load shader fxaa
    {
        shaderFXAA = std::make_shared<Shader>();
        shaderFXAA->setDebugId(
            "data/shaders/filters/fxaa.*.glsl");
        shaderFXAA->loadInternal(
            "data/shaders/filters/fxaa.vert.glsl",
            "data/shaders/filters/fxaa.frag.glsl");
        shaderFXAA->bindTextureLocations({
                { "texColor", 9 }
            });
    }

    // load shader fxaa2
    {
        shaderFXAA2 = std::make_shared<Shader>();
        shaderFXAA2->setDebugId(
            "data/shaders/filters/fxaa2.*.glsl");
        shaderFXAA2->loadInternal(
            "data/shaders/filters/fxaa2.vert.glsl",
            "data/shaders/filters/fxaa2.frag.glsl");
        shaderFXAA2->bindTextureLocations({
                { "texColor", 9 }
            });
    }

    // load shader ssao
    {
        shaderSSAO = std::make_shared<Shader>();
        shaderSSAO->setDebugId(
            "data/shaders/filters/ssao.*.glsl");
        shaderSSAO->loadInternal(
            "data/shaders/filters/ssao.vert.glsl",
            "data/shaders/filters/ssao.frag.glsl");
        shaderSSAO->bindTextureLocations({
                { "texColor", 9 },
                { "texDepth", 5 },
                { "texNormal", 10 }
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
        shaderGeodataColor->setDebugId("data/shaders/geodataColor.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataColor.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataColor.frag.glsl");
        shaderGeodataColor->load(geo + vert.str(), geo + frag.str());
        shaderGeodataColor->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboColorData", 2 }
            });
    }

    // load shader geodata point flat
    {
        shaderGeodataPointFlat = std::make_shared<Shader>();
        shaderGeodataPointFlat->setDebugId(
            "data/shaders/geodataPointFlat.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataPointFlat.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataPoint.frag.glsl");
        shaderGeodataPointFlat->load(geo + vert.str(), geo + frag.str());
        shaderGeodataPointFlat->bindTextureLocations({
                { "texPointData", 0 }
            });
        shaderGeodataPointFlat->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboPointData", 2 }
            });
    }

    // load shader geodata point screen
    {
        shaderGeodataPointScreen = std::make_shared<Shader>();
        shaderGeodataPointScreen->setDebugId(
            "data/shaders/geodataPointScreen.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataPointScreen.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataPoint.frag.glsl");
        shaderGeodataPointScreen->load(geo + vert.str(), geo + frag.str());
        shaderGeodataPointScreen->bindTextureLocations({
                { "texPointData", 0 }
            });
        shaderGeodataPointScreen->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboPointData", 2 }
            });
    }

    // load shader geodata line flat
    {
        shaderGeodataLineFlat = std::make_shared<Shader>();
        shaderGeodataLineFlat->setDebugId(
            "data/shaders/geodataLineFlat.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataLineFlat.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataLine.frag.glsl");
        shaderGeodataLineFlat->load(geo + vert.str(), geo + frag.str());
        shaderGeodataLineFlat->bindTextureLocations({
                { "texLineData", 0 }
            });
        shaderGeodataLineFlat->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboLineData", 2 }
            });
    }

    // load shader geodata line screen
    {
        shaderGeodataLineScreen = std::make_shared<Shader>();
        shaderGeodataLineScreen->setDebugId(
            "data/shaders/geodataLineScreen.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataLineScreen.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataLine.frag.glsl");
        shaderGeodataLineScreen->load(geo + vert.str(), geo + frag.str());
        shaderGeodataLineScreen->bindTextureLocations({
                { "texLineData", 0 }
            });
        shaderGeodataLineScreen->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboLineData", 2 }
            });
    }

    // load shader geodata icon screen
    {
        shaderGeodataIconScreen = std::make_shared<Shader>();
        shaderGeodataIconScreen->setDebugId(
            "data/shaders/geodataIcon.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataIcon.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataIcon.frag.glsl");
        shaderGeodataIconScreen->load(geo + vert.str(), geo + frag.str());
        shaderGeodataIconScreen->bindTextureLocations({
                { "texIcons", 0 }
            });
        shaderGeodataIconScreen->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboIconData", 2 }
            });
    }

    // load shader geodata label flat
    {
        shaderGeodataLabelFlat = std::make_shared<Shader>();
        shaderGeodataLabelFlat->setDebugId(
            "data/shaders/geodataLabelFlat.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataLabelFlat.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataLabelFlat.frag.glsl");
        shaderGeodataLabelFlat->load(geo + vert.str(), geo + frag.str());
        shaderGeodataLabelFlat->bindTextureLocations({
                { "texGlyphs", 0 }
            });
        shaderGeodataLabelFlat->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboLabelFlat", 2 }
            });
        shaderGeodataLabelFlat->loadUniformLocations({
                "uniPass"
            });
    }

    // load shader geodata label screen
    {
        shaderGeodataLabelScreen = std::make_shared<Shader>();
        shaderGeodataLabelScreen->setDebugId(
            "data/shaders/geodataLabelScreen.*.glsl");
        Buffer vert = readInternalMemoryBuffer(
            "data/shaders/geodataLabelScreen.vert.glsl");
        Buffer frag = readInternalMemoryBuffer(
            "data/shaders/geodataLabelScreen.frag.glsl");
        shaderGeodataLabelScreen->load(geo + vert.str(), geo + frag.str());
        shaderGeodataLabelScreen->bindTextureLocations({
                { "texGlyphs", 0 }
            });
        shaderGeodataLabelScreen->bindUniformBlockLocations({
                { "uboCameraData", 0 },
                { "uboViewData", 1 },
                { "uboLabelScreen", 2 }
            });
        shaderGeodataLabelScreen->loadUniformLocations({
                "uniPass"
            });
    }

    // load shader geodata triangle
    {
        shaderGeodataTriangle = std::make_shared<Shader>();
        shaderGeodataTriangle->setDebugId(
            "data/shaders/geodataTriangle.*.glsl");
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

