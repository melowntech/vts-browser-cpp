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

#ifndef FETCHTASK_H_erbgfhufjgf
#define FETCHTASK_H_erbgfhufjgf

#include <string>
#include <atomic>
#include <vts-libs/registry/referenceframe.hpp>
#include <boost/optional.hpp>

#include "include/vts-browser/fetcher.hpp"

namespace vts
{

class MapImpl;

class FetchTaskImpl : public FetchTask
{
public:
    enum class State
    {
        initializing,
        downloading,
        downloaded,
        ready,
        error,
        finalizing,
    };
    
    FetchTaskImpl(MapImpl *map, const std::string &name,
                 FetchTask::ResourceType resourceType);
    virtual ~FetchTaskImpl();

    bool performAvailTest() const;
    bool allowDiskCache() const;
    void saveToCache();
    bool loadFromCache();
    void loadFromInternalMemory();
    virtual void fetchDone() override;

    const std::string name;
    MapImpl *const map;
    boost::optional<vtslibs::registry::BoundLayer::Availability> availTest;
    std::atomic<State> state;
    uint32 redirectionsCount;
};

} // namespace vts

#endif
