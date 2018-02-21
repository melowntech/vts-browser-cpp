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

#include <vts-browser/map.hpp>
#include <vts-browser/options.hpp>

namespace vts { namespace renderer
{

namespace
{

void clearGlState()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGl("cleared gl state");
}

class ShaderAtm : public Shader
{
public:
    ShaderAtm() : firstAtmUniLoc(-1)
    {}

    void initializeAtmosphere()
    {
        firstAtmUniLoc = loadUniformLocations({
            "uniAtmColorLow", "uniAtmColorHigh", "uniAtmParams",
            "uniAtmCameraPosition", "uniAtmViewInv"});
        bindTextureLocations({{"texAtmDensity", 4}});
    }

    uint32 firstAtmUniLoc;
};

} // namespace

class RendererImpl
{
public:
    RenderVariables vars;
    RenderOptions options;
    AtmosphereDensity atmosphere;

    std::shared_ptr<Texture> texCompas;
    std::shared_ptr<Shader> shaderTexture;
    std::shared_ptr<ShaderAtm> shaderSurface;
    std::shared_ptr<ShaderAtm> shaderBackground;
    std::shared_ptr<Shader> shaderInfographic;
    std::shared_ptr<Shader> shaderCopyDepth;
    std::shared_ptr<Mesh> meshQuad; // positions: -1 .. 1
    std::shared_ptr<Mesh> meshRect; // positions: 0 .. 1

    const MapDraws *draws;
    const MapCelestialBody *body;

    mat4 view;
    mat4 proj;
    mat4 viewProj;

    uint32 widthPrev;
    uint32 heightPrev;
    uint32 antialiasingPrev;

    RendererImpl() : draws(nullptr), body(nullptr),
        widthPrev(0), heightPrev(0), antialiasingPrev(0)
    {}

    ~RendererImpl()
    {}

    void drawSurface(const DrawTask &t)
    {
        Texture *tex = (Texture*)t.texColor.get();
        Mesh *m = (Mesh*)t.mesh.get();
        shaderSurface->bind();
        shaderSurface->uniformMat4(1, t.mv);
        shaderSurface->uniformMat3(2, t.uvm);
        shaderSurface->uniformVec4(3, t.color);
        shaderSurface->uniformVec4(4, t.uvClip);
        int flags[4] = {
            t.texMask ? 1 : -1,
            tex->getGrayscale() ? 1 : -1,
            t.flatShading ? 1 : -1,
            t.externalUv ? 1 : -1
        };
        shaderSurface->uniformVec4(5, flags);
        if (t.texMask)
        {
            glActiveTexture(GL_TEXTURE0 + 1);
            ((Texture*)t.texMask.get())->bind();
            glActiveTexture(GL_TEXTURE0 + 0);
        }
        tex->bind();
        m->bind();
        m->dispatch();
    }

    void drawInfographic(const DrawTask &t)
    {
        shaderInfographic->bind();
        shaderInfographic->uniformMat4(1, t.mv);
        shaderInfographic->uniformVec4(2, t.color);
        shaderInfographic->uniform(3, (int)(!!t.texColor));
        if (t.texColor)
        {
            Texture *tex = (Texture*)t.texColor.get();
            tex->bind();
        }
        Mesh *m = (Mesh*)t.mesh.get();
        m->bind();
        m->dispatch();
    }

    void render()
    {
        checkGl("pre-frame check");

        assert(shaderSurface);
        view = rawToMat4(draws->camera.view);
        proj = rawToMat4(draws->camera.proj);
        viewProj = proj * view;

        if (options.width <= 0 || options.height <= 0)
            return;

        // update framebuffer texture
        if (options.width != widthPrev || options.height != heightPrev
                || options.antialiasingSamples != antialiasingPrev)
        {
            widthPrev = options.width;
            heightPrev = options.height;
            antialiasingPrev = std::max(std::min(options.antialiasingSamples,
                             maxAntialiasingSamples), 1u);

            vars.textureTargetType
                    = antialiasingPrev > 1 ? GL_TEXTURE_2D_MULTISAMPLE
                                           : GL_TEXTURE_2D;

            // delete old textures
            glDeleteTextures(1, &vars.depthReadTexId);
            if (vars.depthRenderTexId != vars.depthReadTexId)
                glDeleteTextures(1, &vars.depthRenderTexId);
            glDeleteTextures(1, &vars.colorRenderTexId);
            if (vars.colorRenderTexId != vars.colorReadTexId)
                glDeleteTextures(1, &vars.colorRenderTexId);
            vars.depthReadTexId = vars.depthRenderTexId = 0;
            vars.colorReadTexId = vars.colorRenderTexId = 0;

            // depth texture for rendering
            glActiveTexture(GL_TEXTURE0 + 5);
            glGenTextures(1, &vars.depthRenderTexId);
            glBindTexture(vars.textureTargetType, vars.depthRenderTexId);
            if (antialiasingPrev > 1)
            {
                glTexImage2DMultisample(vars.textureTargetType,
                    antialiasingPrev, GL_DEPTH_COMPONENT32,
                    options.width, options.height, GL_TRUE);
            }
            else
            {
                glTexImage2D(vars.textureTargetType, 0, GL_DEPTH_COMPONENT32,
                             options.width, options.height,
                             0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
                glTexParameteri(vars.textureTargetType,
                                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(vars.textureTargetType,
                                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }
            checkGl("update depth texture for rendering");

            // depth texture for sampling
            glActiveTexture(GL_TEXTURE0 + 6);
            if (antialiasingPrev > 1)
            {
                glGenTextures(1, &vars.depthReadTexId);
                glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                             options.width, options.height,
                             0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST);
            }
            else
            {
                vars.depthReadTexId = vars.depthRenderTexId;
                glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
            }
            checkGl("update depth texture for sampling");

            // color texture for rendering
            glActiveTexture(GL_TEXTURE0 + 7);
            glGenTextures(1, &vars.colorRenderTexId);
            glBindTexture(vars.textureTargetType, vars.colorRenderTexId);
            if (antialiasingPrev > 1)
                glTexImage2DMultisample(
                    vars.textureTargetType, antialiasingPrev, GL_RGB8,
                            options.width, options.height, GL_TRUE);
            else
            {
                glTexImage2D(vars.textureTargetType, 0, GL_RGB8,
                             options.width, options.height,
                             0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(vars.textureTargetType,
                                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(vars.textureTargetType,
                                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }
            checkGl("update color texture for rendering");

            // color texture for sampling
            glActiveTexture(GL_TEXTURE0 + 8);
            if (antialiasingPrev > 1)
            {
                glGenTextures(1, &vars.colorReadTexId);
                glBindTexture(GL_TEXTURE_2D, vars.colorReadTexId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                             options.width, options.height,
                             0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST);
            }
            else
            {
                vars.colorReadTexId = vars.colorRenderTexId;
                glBindTexture(GL_TEXTURE_2D, vars.colorReadTexId);
            }
            checkGl("update color texture for sampling");

            // render frame buffer
            glDeleteFramebuffers(1, &vars.frameRenderBufferId);
            glGenFramebuffers(1, &vars.frameRenderBufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    vars.textureTargetType, vars.depthRenderTexId, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    vars.textureTargetType, vars.colorRenderTexId, 0);
            checkGlFramebuffer();

            // sample frame buffer
            glDeleteFramebuffers(1, &vars.frameReadBufferId);
            glGenFramebuffers(1, &vars.frameReadBufferId);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameReadBufferId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, vars.depthReadTexId, 0);
            checkGlFramebuffer();

            checkGl("update frame buffer");
        }

        // initialize opengl
        glViewport(0, 0, options.width, options.height);
        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        #ifndef VTSR_OPENGLES
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(0, -1000);
        #endif
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkGl("initialized opengl");

        // update shaders
        mat4f projf = rawToMat4(draws->camera.proj).cast<float>();
        shaderInfographic->bind();
        shaderInfographic->uniformMat4(0, projf.data());
        shaderUpdateAtmosphere(shaderSurface.get());
        shaderSurface->uniformMat4(0, projf.data());

        // render opaque
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        for (const DrawTask &t : draws->opaque)
            drawSurface(t);
        checkGl("rendered opaque");

        // render background (atmosphere)
        {
            // corner directions
            vec3 camPos = rawToVec3(draws->camera.eye) / body->majorRadius;
            mat4 inv = (viewProj * scaleMatrix(body->majorRadius)).inverse();
            vec4 cornerDirsD[4] = {
                inv * vec4(-1, -1, 0, 1),
                inv * vec4(+1, -1, 0, 1),
                inv * vec4(-1, +1, 0, 1),
                inv * vec4(+1, +1, 0, 1)
            };
            vec3f cornerDirs[4];
            for (uint32 i = 0; i < 4; i++)
                cornerDirs[i] = normalize(vec4to3(cornerDirsD[i], true)
                                 - camPos).cast<float>();

            shaderUpdateAtmosphere(shaderBackground.get());
            for (uint32 i = 0; i < 4; i++)
                shaderBackground->uniformVec3(i, cornerDirs[i].data());
            meshQuad->bind();
            meshQuad->dispatch();
        }

        // render transparent
        glEnable(GL_BLEND);
        for (const DrawTask &t : draws->transparent)
            drawSurface(t);
        checkGl("rendered transparent");

        // render polygon edges
        if (options.renderPolygonEdges)
        {
            glDisable(GL_BLEND);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (const DrawTask &it : draws->opaque)
            {
                DrawTask t(it);
                t.flatShading = false;
                t.color[0] = t.color[1] = t.color[2] = t.color[3] = 0;
                drawSurface(t);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_BLEND);
            checkGl("rendered polygon edges");
        }

        // copy the depth (resolve multisampling)
        if (vars.depthReadTexId != vars.depthRenderTexId)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
            glBlitFramebuffer(0, 0, options.width, options.height,
                              0, 0, options.width, options.height,
                              GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
            checkGl("copied the depth (resolved multisampling)");
        }
        glDisable(GL_DEPTH_TEST);

        // render infographics
        for (const DrawTask &t : draws->Infographic)
            drawInfographic(t);
        checkGl("rendered infographics");

        // copy the color (resolve multisampling)
        if (options.colorToTexture
                && vars.colorReadTexId != vars.colorRenderTexId)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
            glBlitFramebuffer(0, 0, options.width, options.height,
                              0, 0, options.width, options.height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
            checkGl("copied the color to texture (resolving multisampling)");
        }

        // copy the color to screen
        if (options.colorToTargetFrameBuffer)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, options.targetFrameBuffer);
            glBlitFramebuffer(0, 0, options.width, options.height,
                              options.targetViewportX, options.targetViewportY,
                              options.width, options.height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            checkGl("copied the color to screen (resolving multisampling)");
        }

        // clear the state
        clearGlState();
    }

    void initialize()
    {
        // load texture compas
        {
            texCompas = std::make_shared<Texture>();
            vts::GpuTextureSpec spec(vts::readInternalMemoryBuffer(
                                      "data/textures/compas.png"));
            spec.verticalFlip();
            texCompas->load(spec);
            texCompas->generateMipmaps();
        }

        // load shader texture
        {
            shaderTexture = std::make_shared<Shader>();
            shaderTexture->loadInternal(
                        "data/shaders/texture.vert.glsl",
                        "data/shaders/texture.frag.glsl");
            shaderTexture->loadUniformLocations({"uniMvp", "uniUvm"});
            shaderTexture->bindTextureLocations({{"uniTexture", 0}});
        }

        // load shader surface
        {
            shaderSurface = std::make_shared<ShaderAtm>();
            Buffer vert = readInternalMemoryBuffer(
                        "data/shaders/surface.vert.glsl");
            Buffer atm = readInternalMemoryBuffer(
                        "data/shaders/atmosphere.inc.glsl");
            Buffer frag = readInternalMemoryBuffer(
                        "data/shaders/surface.frag.glsl");
            shaderSurface->load(vert.str(), atm.str() + frag.str());
            shaderSurface->loadUniformLocations({ "uniP", "uniMv", "uniUvMat",
                "uniColor", "uniUvClip", "uniFlags" });
            shaderSurface->bindTextureLocations({{"texColor", 0}, {"texMask", 1}});
            shaderSurface->initializeAtmosphere();
        }

        // load shader background
        {
            shaderBackground = std::make_shared<ShaderAtm>();
            Buffer vert = readInternalMemoryBuffer(
                        "data/shaders/background.vert.glsl");
            Buffer atm = readInternalMemoryBuffer(
                        "data/shaders/atmosphere.inc.glsl");
            Buffer frag = readInternalMemoryBuffer(
                        "data/shaders/background.frag.glsl");
            shaderBackground->load(vert.str(), atm.str() + frag.str());
            shaderBackground->loadUniformLocations({"uniCorners[0]",
                "uniCorners[1]","uniCorners[2]","uniCorners[3]"});
            shaderBackground->initializeAtmosphere();
        }

        // load shader infographic
        {
            shaderInfographic = std::make_shared<Shader>();
            shaderInfographic->loadInternal(
                        "data/shaders/infographic.vert.glsl",
                        "data/shaders/infographic.frag.glsl");
            shaderInfographic->loadUniformLocations({"uniP", "uniMv", "uniColor",
                                                     "uniUseColorTexture"});
            shaderInfographic->bindTextureLocations({{"texColor", 0},
                                                     {"texDepth", 6}});
        }

        // load shader copy depth
        {
            shaderCopyDepth = std::make_shared<Shader>();
            shaderCopyDepth->loadInternal(
                        "data/shaders/copyDepth.vert.glsl",
                        "data/shaders/copyDepth.frag.glsl");
            shaderCopyDepth->loadUniformLocations({"uniTexPos"});
            shaderCopyDepth->bindTextureLocations({{"texDepth", 0}});
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
            meshQuad->load(spec);
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
            meshRect->load(spec);
        }

        atmosphere.initialize();
    }

    void finalize()
    {
        texCompas.reset();
        shaderTexture.reset();
        shaderSurface.reset();
        shaderBackground.reset();
        shaderInfographic.reset();
        shaderCopyDepth.reset();
        meshQuad.reset();
        meshRect.reset();

        atmosphere.finalize();

        if (vars.frameRenderBufferId)
        {
            glDeleteFramebuffers(1, &vars.frameRenderBufferId);
            vars.frameRenderBufferId = 0;
        }

        if (vars.frameReadBufferId)
        {
            glDeleteFramebuffers(1, &vars.frameReadBufferId);
            vars.frameReadBufferId = 0;
        }

        if (vars.depthRenderTexId)
        {
            glDeleteTextures(1, &vars.depthRenderTexId);
            vars.depthRenderTexId = 0;
        }

        if (vars.depthReadTexId)
        {
            glDeleteTextures(1, &vars.depthReadTexId);
            vars.depthReadTexId = 0;
        }

        if (vars.colorRenderTexId)
        {
            glDeleteTextures(1, &vars.colorRenderTexId);
            vars.colorRenderTexId = 0;
        }

        widthPrev = heightPrev = antialiasingPrev = 0;
    }

    void shaderUpdateAtmosphere(ShaderAtm *shader)
    {
        shader->bind();
        float p[4] = {0,0,0,0};
        shader->uniformVec4(shader->firstAtmUniLoc + 2, p);

        if (!options.renderAtmosphere)
            return;

        Texture *tex = atmosphere.validate(*body);
        if (!tex)
            return;

        // uniParams
        float uniAtmParams[4] = {
            (float)(body->atmosphere.thickness / body->majorRadius),
            (float)body->atmosphere.horizontalExponent,
            (float)(body->minorRadius / body->majorRadius),
            (float)body->majorRadius        };

        // camera position
        vec3 camPos = rawToVec3(draws->camera.eye) / body->majorRadius;
        vec3f uniAtmCameraPosition = camPos.cast<float>();

        // view inv
        mat4 vi = rawToMat4(draws->camera.view).inverse();
        mat4f uniAtmViewInv = vi.cast<float>();

        // upload shader uniforms
        uint32 loc = shader->firstAtmUniLoc;
        shader->uniformVec4(loc + 0, body->atmosphere.colorLow);
        shader->uniformVec4(loc + 1, body->atmosphere.colorHigh);
        shader->uniformVec4(loc + 2, uniAtmParams);
        shader->uniformVec3(loc + 3, (float*)uniAtmCameraPosition.data());
        shader->uniformMat4(loc + 4, (float*)uniAtmViewInv.data());

        // bind atmosphere density texture
        glActiveTexture(GL_TEXTURE4);
        tex->bind();
        glActiveTexture(GL_TEXTURE0);
    }

    void renderCompass(const double screenPosSize[3],
                       const double mapRotation[3])
    {
        glEnable(GL_BLEND);
        glActiveTexture(GL_TEXTURE0);
        texCompas->bind();
        shaderTexture->bind();
        mat4 p = orthographicMatrix(-1, 1, -1, 1, -1, 1)
                * scaleMatrix(1.0 / widthPrev, 1.0 / heightPrev, 1);
        mat4 v = translationMatrix(screenPosSize[0] * 2 - widthPrev,
                                    screenPosSize[1] * 2 - heightPrev, 0)
                * scaleMatrix(screenPosSize[2], screenPosSize[2], 1);
        mat4 m = rotationMatrix(0, mapRotation[1] + 90)
             * rotationMatrix(2, mapRotation[0]);
        mat4f mvpf = (p * v * m).cast<float>();
        mat3f uvmf = identityMatrix3().cast<float>();
        shaderTexture->uniformMat4(0, mvpf.data());
        shaderTexture->uniformMat3(1, uvmf.data());
        meshQuad->bind();
        meshQuad->dispatch();
    }

    void getWorldPosition(const double screenPos[2], double worldPos[3])
    {
        double x = screenPos[0];
        double y = screenPos[1];
        y = heightPrev - y - 1;

        float depth = std::numeric_limits<float>::quiet_NaN();
        #ifdef VTSR_OPENGLES
        // opengl ES does not support reading depth with glReadPixels
        {
            clearGlState();
            uint32 fbId = 0, texId = 0;

            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            glGenFramebuffers(1, &fbId);
            glBindFramebuffer(GL_FRAMEBUFFER, fbId);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texId, 0);
            checkGlFramebuffer();

            shaderCopyDepth->bind();
            int pos[2] = { (int)x, (int)y };
            shaderCopyDepth->uniformVec2(0, pos);
            glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
            meshQuad->bind();
            meshQuad->dispatch();

            unsigned char res[4];
            glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, res);
            if (res[0] != 0 || res[1] != 0 || res[2] != 0 || res[3] != 0)
            {
                static const vec4 bitSh = vec4(1.0 / (256.0*256.0*256.0),
                                               1.0 / (256.0*256.0),
                                               1.0 / 256.0, 1.0);
                depth = 0;
                for (int i = 0; i < 4; i++)
                    depth += res[i] * bitSh[i];
                depth /= 255;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbId);
            glDeleteTextures(1, &texId);
        }
        #else
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameReadBufferId);
            glReadPixels((int)x, (int)y, 1, 1,
                     GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        }
        #endif
        checkGl("read depth");
        clearGlState();

        if (depth > 1 - 1e-7)
            depth = std::numeric_limits<float>::quiet_NaN();
        depth = depth * 2 - 1;
        x = x / widthPrev * 2 - 1;
        y = y / heightPrev * 2 - 1;
        vec3 res = vec4to3(viewProj.inverse() * vec4(x, y, depth, 1), true);
        for (int i = 0; i < 3; i++)
            worldPos[i] = res[i];
    }
};

RenderOptions::RenderOptions() : width(0), height(0),
    targetFrameBuffer(0), targetViewportX(0), targetViewportY(0),
    antialiasingSamples(1),
    renderAtmosphere(true), renderPolygonEdges(false),
    colorToTargetFrameBuffer(true), colorToTexture(false)
{}

RenderVariables::RenderVariables() :
    frameRenderBufferId(0), frameReadBufferId(0),
    depthRenderTexId(0), depthReadTexId(0),
    colorRenderTexId(0),
    textureTargetType(0)
{}

Renderer::Renderer()
{
    impl = std::make_shared<RendererImpl>();
}

Renderer::~Renderer()
{}

void Renderer::initialize()
{
    impl->initialize();
}

void Renderer::finalize()
{
    impl->finalize();
}

void Renderer::loadTexture(ResourceInfo &info, GpuTextureSpec &spec)
{
    auto r = std::make_shared<Texture>();
    r->load(info, spec);
    info.userData = r;
}

void Renderer::loadMesh(ResourceInfo &info, GpuMeshSpec &spec)
{
    auto r = std::make_shared<Mesh>();
    r->load(info, spec);
    info.userData = r;
}

void Renderer::bindLoadFunctions(Map *map)
{
    map->callbacks().loadTexture = std::bind(&Renderer::loadTexture, this,
            std::placeholders::_1, std::placeholders::_2);
    map->callbacks().loadMesh = std::bind(&Renderer::loadMesh, this,
            std::placeholders::_1, std::placeholders::_2);
}

RenderOptions &Renderer::options()
{
    return impl->options;
}

const RenderVariables &Renderer::variables() const
{
    return impl->vars;
}

void Renderer::render(const MapDraws &draws,
            const MapCelestialBody &body)
{
    impl->draws = &draws;
    impl->body = &body;
    impl->render();
    impl->draws = nullptr;
    impl->body = nullptr;
}

void Renderer::render(Map *map)
{
    render(map->draws(), map->celestialBody());
}

void Renderer::renderCompass(const double screenPosSize[3],
                   const double mapRotation[3])
{
    impl->renderCompass(screenPosSize, mapRotation);
}

void Renderer::getWorldPosition(const double screenPosIn[2],
                      double worldPosOut[3])
{
    impl->getWorldPosition(screenPosIn, worldPosOut);
}

} } // namespace vts renderer

