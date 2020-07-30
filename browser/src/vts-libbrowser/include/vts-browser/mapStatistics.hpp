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

#ifndef MAP_STATISTICS_HPP_wqieufhbvgjh
#define MAP_STATISTICS_HPP_wqieufhbvgjh

#include <string>

#include "foundation.hpp"

namespace vts
{

class VTS_API MapStatistics
{
public:
    std::string toJson() const;

    uint32 resourcesCreated = 0;
    uint32 resourcesDownloaded = 0;
    uint32 resourcesDiskLoaded = 0;
    uint32 resourcesDecoded = 0;
    uint32 resourcesUploaded = 0;
    uint32 resourcesFailed = 0;
    uint32 resourcesReleased = 0;

    uint32 resourcesExists = 0;
    uint32 resourcesActive = 0;
    uint32 resourcesDownloading = 0;
    uint32 resourcesPreparing = 0;
    uint32 resourcesQueueCacheRead = 0;
    uint32 resourcesQueueCacheWrite = 0;
    uint32 resourcesQueueDownload = 0;
    uint32 resourcesQueueDecode = 0;
    uint32 resourcesQueueUpload = 0;
    uint32 resourcesQueueAtmosphere = 0;
    uint32 resourcesAccessed = 0;

    uint32 currentGpuMemUseKB = 0;
    uint32 currentRamMemUseKB = 0;

    uint32 renderTicks = 0;
};

} // namespace vts

#endif
