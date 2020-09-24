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

#ifndef MAPVIEW_HPP_wqfzugsahakwejgr
#define MAPVIEW_HPP_wqfzugsahakwejgr

#include <string>
#include <vector>
#include <map>

#include "foundation.hpp"

namespace vts
{

class VTS_API MapView
{
public:
    class VTS_API BoundLayerInfo
    {
    public:
        std::string id;
        double alpha = 1;
    };

    class VTS_API SurfaceInfo
    {
    public:
        std::vector<BoundLayerInfo> boundLayers;
    };

    class VTS_API FreeLayerInfo
    {
    public:
        std::vector<BoundLayerInfo> boundLayers;
        std::string styleUrl;
    };

    std::string description;
    std::map<std::string, SurfaceInfo> surfaces;
    std::map<std::string, FreeLayerInfo> freeLayers;


    MapView() = default;
    explicit MapView(const std::string &str);
    std::string toJson() const;
    std::string toUrl() const;
};

} // namespace vts

#endif
