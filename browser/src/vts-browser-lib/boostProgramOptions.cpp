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

#include <boost/algorithm/string.hpp>
#include <utility/enum-io.hpp>
#include "include/vts-browser/boostProgramOptions.hpp"
#include "include/vts-browser/options.hpp"
#include "include/vts-browser/fetcher.hpp"
#include "include/vts-browser/log.hpp"

namespace po = boost::program_options;

namespace vts
{

namespace
{

void notifyLogMask(const std::string &mask)
{
    setLogMask(mask);
}

void notifyLogFile(const std::string &name)
{
    setLogFile(name);
}

void notifyLogConsole(bool console)
{
    setLogConsole(console);
}

void sanitizeSection(std::string &s)
{
    if (s.empty())
        return;
    if (s[s.size() - 1] != '.')
        s += '.';
}

} // namespace

UTILITY_GENERATE_ENUM_IO(TraverseMode,
                         ((Hierarchical)("hierarchical"))
                         ((Flat)("flat"))
                         )

void optionsConfigLog(
        boost::program_options::options_description &desc,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "mask").c_str(),
        po::value<std::string>()->notifier(&notifyLogMask),
        "Set log mask.\n"
        "Format: I?E?W?\n"
        "Eg.: I3W1E4")

    ((section + "file").c_str(),
        po::value<std::string>()->notifier(&notifyLogFile),
        "Set log output file path.")

    ((section + "console").c_str(),
        po::value<bool>()->notifier(&notifyLogConsole),
        "Enable log output to console.")
    ;
}

void optionsConfigCreateOptions(
        boost::program_options::options_description &desc,
        MapCreateOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "clientId").c_str(),
        po::value<std::string>(&opts->clientId),
        "Identification of the application for the authentication server.")

    ((section + "cachePath").c_str(),
        po::value<std::string>(&opts->cachePath),
        "Path to a directory where all downloaded resources are cached.")

    ((section + "disableCache").c_str(),
        po::value<bool>(&opts->disableCache),
        "Set to yes to completly disable the cache.")
    ;
}

void optionsConfigMapOptions(
        boost::program_options::options_description &desc,
        MapOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "maxTexelToPixelScale").c_str(),
        po::value<double>(&opts->maxTexelToPixelScale),
        "Maximum ratio of texture details to the viewport resolution.")

    ((section + "maxResourcesMemory").c_str(),
        po::value<uint64>(&opts->maxResourcesMemory),
        "Maximum memory (in bytes) used by resources "
        "before they begin to unload.")

    ((section + "maxConcurrentDownloads").c_str(),
        po::value<uint32>(&opts->maxConcurrentDownloads),
        "Maximum size of the queue for the resources to be downloaded.")

    ((section + "maxNodeMetaUpdatesPerTick").c_str(),
        po::value<uint32>(&opts->maxNodeMetaUpdatesPerTick),
        "Maximum number of node meta-data updates per frame.")

    ((section + "maxNodeDrawsUpdatesPerTick").c_str(),
        po::value<uint32>(&opts->maxNodeDrawsUpdatesPerTick),
        "Maximum number of node render-data updates per frame.")

    ((section + "maxResourceProcessesPerTick").c_str(),
        po::value<uint32>(&opts->maxResourceProcessesPerTick),
        "Maximum number of resources processed per dataTick.")

    ((section + "maxFetchRedirections").c_str(),
        po::value<uint32>(&opts->maxFetchRedirections),
        "Maximum number of redirections before the download fails.")

    ((section + "maxFetchRetries").c_str(),
        po::value<uint32>(&opts->maxFetchRetries),
        "Maximum number of attempts to redownload a resource.")

    ((section + "fetchFirstRetryTimeOffset").c_str(),
        po::value<uint32>(&opts->fetchFirstRetryTimeOffset),
        "Delay in seconds for first resource download retry.")

    ((section + "traverseMode").c_str(),
        po::value<TraverseMode>(&opts->traverseMode),
        "Render traversal mode:\n"
        "hierarchical\n"
        "flat")
    ;
}

void optionsConfigDebugOptions(
        boost::program_options::options_description &desc,
        MapOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "debugDetachedCamera").c_str(),
        po::value<bool>(&opts->debugDetachedCamera),
        "debugDetachedCamera")

    ((section + "debugDisableVirtualSurfaces").c_str(),
        po::value<bool>(&opts->debugDisableVirtualSurfaces),
        "debugDisableVirtualSurfaces")

    ((section + "debugDisableSri").c_str(),
        po::value<bool>(&opts->debugDisableSri),
        "debugDisableSri")

    ((section + "debugSaveCorruptedFiles").c_str(),
        po::value<bool>(&opts->debugSaveCorruptedFiles),
        "debugSaveCorruptedFiles")

    ((section + "debugFlatShading").c_str(),
        po::value<bool>(&opts->debugFlatShading),
        "debugFlatShading")

    ((section + "debugRenderSurrogates").c_str(),
        po::value<bool>(&opts->debugRenderSurrogates),
        "debugRenderSurrogates")

    ((section + "debugRenderMeshBoxes").c_str(),
        po::value<bool>(&opts->debugRenderMeshBoxes),
        "debugRenderMeshBoxes")

    ((section + "debugRenderTileBoxes").c_str(),
        po::value<bool>(&opts->debugRenderTileBoxes),
        "debugRenderTileBoxes")

    ((section + "debugRenderObjectPosition").c_str(),
        po::value<bool>(&opts->debugRenderObjectPosition),
        "debugRenderObjectPosition")

    ((section + "debugRenderTargetPosition").c_str(),
        po::value<bool>(&opts->debugRenderTargetPosition),
        "debugRenderTargetPosition")

    ((section + "debugRenderAltitudeShiftCorners").c_str(),
        po::value<bool>(&opts->debugRenderAltitudeShiftCorners),
        "debugRenderAltitudeShiftCorners")

    ((section + "debugRenderNoMeshes").c_str(),
        po::value<bool>(&opts->debugRenderNoMeshes),
        "debugRenderNoMeshes")
    ;
}

void optionsConfigFetcherOptions(
        boost::program_options::options_description &desc,
        FetcherOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "threads").c_str(),
        po::value<uint32>(&opts->threads),
        "Number of threads created for the fetcher.")

    ((section + "maxHostConnections").c_str(),
        po::value<uint32>(&opts->maxHostConnections),
        "Maximum concurrent connections to same host.")

    ((section + "maxTotalConections").c_str(),
        po::value<uint32>(&opts->maxTotalConections),
        "Total limit of concurrent connections.")

    ((section + "maxCacheConections").c_str(),
        po::value<uint32>(&opts->maxCacheConections),
        "Size of curl connection cache.")

    ((section + "pipelining").c_str(),
        po::value<sint32>(&opts->pipelining),
        "HTTP pipelining mode.")
    ;
}

} // namespace vts
