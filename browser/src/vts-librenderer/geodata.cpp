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

namespace priv
{

class Geodata
{
public:
    void load(ResourceInfo &info, GpuGeodataSpec &specp)
    {
        this->spec = std::move(specp);

        mesh = std::make_shared<Mesh>();
        ResourceInfo mshInfo;
        mesh->load(mshInfo, spec.createMesh());
        info.gpuMemoryCost += mshInfo.gpuMemoryCost;
        info.ramMemoryCost += mshInfo.ramMemoryCost;
        std::vector<std::array<float, 3>>().swap(spec.coordinates);

        texture = std::static_pointer_cast<Texture>(spec.texture);
        font = std::static_pointer_cast<Font>(spec.font);

        info.ramMemoryCost += sizeof(spec);
    }

    GpuGeodataSpec spec;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Font> font;
};

void RendererImpl::initializeGeodata()
{
    // load shader geodata
    {
        shaderGeodata = std::make_shared<Shader>();
        shaderGeodata->loadInternal(
            "data/shaders/geodata.vert.glsl",
            "data/shaders/geodata.frag.glsl");
        shaderGeodata->loadUniformLocations(
            {
                // rendering
                "uniMv", "uniMvp",
                // common
                "uniType", "uniColor"
            });
    }
}

void RendererImpl::renderGeodata()
{
    //glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    shaderGeodata->bind();

    // the offset is applied as multiplicator in clip-space
    //   and is dependant on near and far planes
    //   which are different here than in the js browser
    //   therefore a different method should be found
    /*
    vec3 zBufferOffsetValues;
    {
        vec3 up1 = normalize(rawToVec3(draws->camera.eye)); // todo projected systems
        vec3 up2 = vec4to3(vec4(viewInv * vec4(0, 1, 0, 0)));
        double tiltFactor = std::acos(std::max(
            dot(up1, up2), 0.0)) / M_PI_2;

        double distance = draws->camera.tagretDistance;
        double distanceFactor = 1 / std::max(1.0,
            std::log(distance) / std::log(1.04));

        zBufferOffsetValues = vec3(1, distanceFactor, tiltFactor);
    }
    */

    for (const DrawGeodataTask &t : draws->geodata)
    {
        Geodata *g = (Geodata*)t.geodata.get();
        Mesh *msh = (Mesh*)g->mesh.get();
        if (!msh)
            return;

        /*
        mat4 depthOffset;
        {
            vec3 zbo = rawToVec3(g->spec.commonData.zBufferOffset)
                                            .cast<double>();
            double off = dot(zbo, zBufferOffsetValues) * 0.0001;
            double off2 = (off + 1) * 2 - 1;
            depthOffset = scaleMatrix(vec3(1, 1, off2));
        }
        */

        // transformation
        mat4 model = rawToMat4(g->spec.model);
        shaderGeodata->uniformMat4(0,
                mat4f((view * model).cast<float>()).data());
        mat4 mvp = viewProj * model;
        //mat4 mvpz = depthOffset * mvp;
        mat4 mvpz = mvp;
        shaderGeodata->uniformMat4(1, mat4f(mvpz.cast<float>()).data());

        // common data
        shaderGeodata->uniform(2, (int)g->spec.type);

        // union data
        switch (g->spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        {
            glLineWidth(g->spec.unionData.line.width);
            shaderGeodata->uniformVec4(3,
                rawToVec4(g->spec.unionData.line.color).data());
        } break;
        case GpuGeodataSpec::Type::LineWorld:
        {
            shaderGeodata->uniformVec4(3,
                rawToVec4(g->spec.unionData.line.color).data());
        } break;
        case GpuGeodataSpec::Type::PointScreen:
        {
            glPointSize(g->spec.unionData.point.radius);
            shaderGeodata->uniformVec4(3,
                rawToVec4(g->spec.unionData.point.color).data());
        } break;
        case GpuGeodataSpec::Type::PointWorld:
        {
            shaderGeodata->uniformVec4(3,
                rawToVec4(g->spec.unionData.point.color).data());
        } break;
        default:
        {
            shaderGeodata->uniformVec4(2, vec4f(1,1,1,1).data());
        }
        }

        // dispatch
        msh->bind();
        msh->dispatch();

        // debug
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        //msh->dispatch();
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glLineWidth(1);
    glPointSize(1);

    glDepthMask(GL_TRUE);
    //glEnable(GL_CULL_FACE);
}

} // namespace priv

void Renderer::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec)
{
    auto r = std::make_shared<Geodata>();
    r->load(info, spec);
    info.userData = r;
}

} } // namespace vts renderer

