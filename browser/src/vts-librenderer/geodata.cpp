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
//#include <vts-browser/map.hpp>
//#include <vts-browser/mapCallbacks.hpp>
//#include <vts-browser/camera.hpp>
#include <vts-browser/cameraDraws.hpp>
//#include <vts-browser/celestial.hpp>

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
        mesh->load(spec.createMesh());
        texture = std::static_pointer_cast<Texture>(spec.texture);
        font = std::static_pointer_cast<Font>(spec.font);
    }

    GpuGeodataSpec spec;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Font> font;
};

void RendererImpl::renderGeodata()
{
    shaderInfographic->bind();
    for (const DrawGeodataTask &t : draws->geodata)
    {
        Geodata *g = (Geodata*)t.geodata.get();
        Mesh *m = (Mesh*)g->mesh.get();
        if (!m)
            return;
        mat4f mv = (view * rawToMat4(g->spec.model)).cast<float>();
        shaderInfographic->uniformMat4(1, mv.data());
        switch (g->spec.type)
        {
        case GpuGeodataSpec::Type::LineScreen:
        case GpuGeodataSpec::Type::LineWorld:
            shaderInfographic->uniformVec4(2,
                rawToVec4(g->spec.sharedData.line.color).data());
            break;
        case GpuGeodataSpec::Type::PointScreen:
        case GpuGeodataSpec::Type::PointWorld:
            shaderInfographic->uniformVec4(2,
                rawToVec4(g->spec.sharedData.point.color).data());
            break;
        default:
            shaderInfographic->uniformVec4(2, vec4f(1,1,1,1).data());
        }
        shaderInfographic->uniform(3, 0);
        m->bind();
        m->dispatch();
    }
}

} // namespace priv

void Renderer::loadGeodata(ResourceInfo &info, GpuGeodataSpec &spec)
{
    auto r = std::make_shared<Geodata>();
    r->load(info, spec);
    info.userData = r;
}

} } // namespace vts renderer

