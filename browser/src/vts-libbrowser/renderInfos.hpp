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

#ifndef RENDERINFOS_HPP_c1de5gr46947
#define RENDERINFOS_HPP_c1de5gr46947

#include <vts-libs/registry/referenceframe.hpp>
#include <vts-libs/registry/freelayer.hpp>
#include <vts-libs/vts/urltemplate.hpp>

#include "include/vts-browser/math.hpp"

namespace vts
{

using TileId = vtslibs::registry::ReferenceFrame::Division::Node::Id;
using vtslibs::vts::UrlTemplate;

enum class Validity;

class GeodataStylesheet;
class CameraImpl;
class GpuTexture;
class MapImpl;

class BoundInfo : public vtslibs::registry::BoundLayer
{
public:
    BoundInfo(const vtslibs::registry::BoundLayer &bl,
        const std::string &url);

    std::shared_ptr<vtslibs::registry::BoundLayer::Availability>
        availability;
    UrlTemplate urlExtTex;
    UrlTemplate urlMeta;
    UrlTemplate urlMask;
};

class FreeInfo : public vtslibs::registry::FreeLayer
{
public:
    FreeInfo(const vtslibs::registry::FreeLayer &fl,
        const std::string &url);

    std::string url; // external free layer url

    std::shared_ptr<GeodataStylesheet> stylesheet;
    std::shared_ptr<const std::string> overrideGeodata; // monolithic only
};

class BoundParamInfo : public vtslibs::registry::View::BoundLayerParams
{
public:
    typedef std::vector<BoundParamInfo> List;

    BoundParamInfo(const vtslibs::registry::View::BoundLayerParams &params);
    vec4f uvTrans() const;
    Validity prepare(CameraImpl *impl, TileId tileId, TileId localId,
        uint32 subMeshIndex, double priority);

    std::shared_ptr<GpuTexture> textureColor;
    std::shared_ptr<GpuTexture> textureMask;
    const BoundInfo *bound = nullptr;
    bool transparent = false;

private:
    Validity prepareDepth(CameraImpl *impl, double priority);

    UrlTemplate::Vars orig {0};
    sint32 depth = 0;
};

} // namespace vts

#endif
