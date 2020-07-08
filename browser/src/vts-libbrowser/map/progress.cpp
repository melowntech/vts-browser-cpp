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

#include "../camera.hpp"
#include "../map.hpp"

namespace vts
{

namespace
{

uint32 getActive(const MapImpl * map)
{
    uint32 active = map->statistics.resourcesPreparing;
    for (auto &camera : map->cameras)
    {
        auto cam = camera.lock();
        if (cam)
        {
            active += cam->statistics.currentNodeMetaUpdates;
            active += cam->statistics.currentNodeDrawsUpdates;
        }
    }
    return active;
}

} // namespace

bool MapImpl::getMapRenderComplete()
{
    return getActive(this) == 0;
}

double MapImpl::getMapRenderProgress()
{
    uint32 active = getActive(this);
    if (active == 0)
    {
        progressEstimationMaxResources = 0;
        return 0;
    }
    progressEstimationMaxResources
            = std::max(progressEstimationMaxResources, active);
    return double(progressEstimationMaxResources - active)
            / progressEstimationMaxResources;
}

} // namespace vts
