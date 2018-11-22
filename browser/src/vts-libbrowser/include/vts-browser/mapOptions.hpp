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

#ifndef MAP_OPTIONS_HPP_kwegfdzvgsdfj
#define MAP_OPTIONS_HPP_kwegfdzvgsdfj

#include <string>

#include "foundation.hpp"

namespace vts
{

// these options are passed to the map when it is beeing created
//   and are immutable during the lifetime of the map
class VTS_API MapCreateOptions
{
public:
    MapCreateOptions();
    MapCreateOptions(const std::string &json);
    void applyJson(const std::string &json);
    std::string toJson() const;

    // application id that is sent with all resources
    // to be used by authentication servers
    std::string clientId;

    // path to a directory in which to store all downloaded files
    // leave it empty to use default ($HOME/.cache/vts-browser)
    std::string cachePath;

    // search url/srs are usually provided in browser options in mapconfig
    // however, if they are not provided, these fallbacks are used
    std::string searchUrlFallback;
    std::string searchSrsFallback;

    // custom srs definitions that you may use
    //   for additional coordinate transformations
    std::string customSrs1;
    std::string customSrs2;

    // use hard drive cache for downloads
    bool diskCache;

    // true -> use new scheme for naming (hashing) files
    //         in a hierarchy of directories in the cache
    // false -> use old scheme where the name of the downloaded resource
    //          is clearly reflected in the cached file name
    bool hashCachePaths;

    // use search url/srs fallbacks on any body (not just Earth)
    bool searchUrlFallbackOutsideEarth;

    // use search url/srs from mapconfig
    bool browserOptionsSearchUrls;

    // provide density texture for atmosphere rendering
    bool atmosphereDensityTexture;
};

// options of the map which may be changed anytime
// (although, some of the options may take effect slowly
//    as some internal caches are beeing rebuild)
class VTS_API MapRuntimeOptions
{
public:
    MapRuntimeOptions();
    MapRuntimeOptions(const std::string &json);
    void applyJson(const std::string &json);
    std::string toJson() const;

    // relative scale of every tile.
    // small up-scale may reduce occasional holes on tile borders.
    double renderTilesScale;

    // memory threshold at which resources start to be released
    uint32 targetResourcesMemoryKB;

    // maximum size of the queue for the resources to be downloaded
    uint32 maxConcurrentDownloads;

    // maximum number of resources processed per dataTick
    uint32 maxResourceProcessesPerTick;

    // maximum number of redirections before the download fails
    // this is to prevent infinite loops
    uint32 maxFetchRedirections;

    // maximum number of attempts to redownload a resource
    uint32 maxFetchRetries;

    // delay in seconds for first resource download retry
    // each subsequent retry is delayed twice as long as before
    uint32 fetchFirstRetryTimeOffset;

    // to improve search results relevance, the results are further
    //   filtered and reordered
    bool searchResultsFiltering;

    bool debugVirtualSurfaces;
    bool debugSaveCorruptedFiles;
    bool debugValidateGeodataStyles;
    bool debugCoarsenessDisks;
    bool debugExtractRawResources;
};

} // namespace vts

#endif
