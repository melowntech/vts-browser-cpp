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
#include <dbglog/dbglog.hpp>

#include "../include/vts-browser/boostProgramOptions.hpp"
#include "../include/vts-browser/mapOptions.hpp"
#include "../include/vts-browser/cameraOptions.hpp"
#include "../include/vts-browser/navigationOptions.hpp"
#include "../include/vts-browser/fetcher.hpp"
#include "../include/vts-browser/log.hpp"

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

#define FILE_OPTIONS \
    ((section + "file").c_str(), \
        po::value<std::string>()->notifier( \
            [&](const std::string &name) { \
                opts->applyJson( \
                    vts::readLocalFileBuffer(name).str()); \
            }), \
        "Path to file with additional JSON encoded options.")


void optionsConfigLog(
        boost::program_options::options_description &desc,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "mask").c_str(),
        po::value<std::string>()->notifier(&notifyLogMask)
        ->default_value(dbglog::get_mask_string()),
        "Set log mask.\n"
        "Format: I?E?W?\n"
        "Eg.: I3W1E4")

    ((section + "file").c_str(),
        po::value<std::string>()->notifier(&notifyLogFile),
        "Set log output file path.")

    ((section + "console").c_str(),
        po::value<bool>()->notifier(&notifyLogConsole)
        ->default_value(dbglog::get_log_console())
        ->implicit_value(!dbglog::get_log_console()),
        "Enable log output to console.")
    ;
}

void optionsConfigMapCreate(
    boost::program_options::options_description &desc,
    MapCreateOptions *opts,
    std::string section)
{
    sanitizeSection(section);
    desc.add_options()
    FILE_OPTIONS

    ((section + "clientId").c_str(),
        po::value<std::string>(&opts->clientId)
        ->default_value(opts->clientId),
        "Identification of the application for the authentication server.")

    ((section + "cachePath").c_str(),
        po::value<std::string>(&opts->cachePath)
        ->default_value(opts->cachePath),
        "Path to a directory where all downloaded resources are cached.")

    ((section + "diskCache").c_str(),
        po::value<bool>(&opts->diskCache)
        ->default_value(opts->diskCache)
        ->implicit_value(!opts->diskCache),
        "Use disk cache.")
    ;
}

void optionsConfigMapRuntime(
    boost::program_options::options_description &desc,
    MapRuntimeOptions *opts,
    std::string section)
{
    sanitizeSection(section);
    desc.add_options()
    FILE_OPTIONS

    ((section + "renderTilesScale").c_str(),
        po::value<double>(&opts->renderTilesScale)
        ->default_value(opts->renderTilesScale),
        "Scale of every tile. "
        "Small up-scale may reduce occasional holes on tile borders.")

    ((section + "targetResourcesMemoryKB").c_str(),
        po::value<uint32>(&opts->targetResourcesMemoryKB)
        ->default_value(opts->targetResourcesMemoryKB),
        "Target memory (in KB) used by resources "
        "before they begin to unload.")

    ((section + "maxConcurrentDownloads").c_str(),
        po::value<uint32>(&opts->maxConcurrentDownloads)
        ->default_value(opts->maxConcurrentDownloads),
        "Maximum size of the queue for the resources to be downloaded.")

    ((section + "maxFetchRedirections").c_str(),
        po::value<uint32>(&opts->maxFetchRedirections)
        ->default_value(opts->maxFetchRedirections),
        "Maximum number of redirections before the download fails.")

    ((section + "maxFetchRetries").c_str(),
        po::value<uint32>(&opts->maxFetchRetries)
        ->default_value(opts->maxFetchRetries),
        "Maximum number of attempts to redownload a resource.")

    ((section + "fetchFirstRetryTimeOffset").c_str(),
        po::value<uint32>(&opts->fetchFirstRetryTimeOffset)
        ->default_value(opts->fetchFirstRetryTimeOffset),
        "Delay in seconds for first resource download retry.")

    ((section + "debugSaveCorruptedFiles").c_str(),
        po::value<bool>(&opts->debugSaveCorruptedFiles)
        ->default_value(opts->debugSaveCorruptedFiles)
        ->implicit_value(!opts->debugSaveCorruptedFiles),
        "debugSaveCorruptedFiles")
    ;
}

void optionsConfigCamera(
    boost::program_options::options_description &desc,
    CameraOptions *opts,
    std::string section)
{
    sanitizeSection(section);
    desc.add_options()
    FILE_OPTIONS

    ((section + "maxTexelToPixelScale").c_str(),
        po::value<double>(&opts->maxTexelToPixelScale)
        ->default_value(opts->maxTexelToPixelScale),
        "Maximum ratio of texture details to the viewport resolution.")

    ((section + "traverseModeSurfaces").c_str(),
        po::value<TraverseMode>(&opts->traverseModeSurfaces)
        ->default_value(opts->traverseModeSurfaces),
        "Render traversal mode for surfaces:\n"
        "none\n"
        "flat\n"
        "stable\n"
        "balanced\n"
        "hierarchical\n"
        "fixed")

    ((section + "traverseModeGeodata").c_str(),
        po::value<TraverseMode>(&opts->traverseModeGeodata)
        ->default_value(opts->traverseModeGeodata),
        "Render traversal mode for geodata:\n"
        "none\n"
        "flat\n"
        "stable\n"
        "balanced\n"
        "hierarchical\n"
        "fixed")

    ((section + "balancedGridLodOffset").c_str(),
        po::value<uint32>(&opts->balancedGridLodOffset)
        ->default_value(opts->balancedGridLodOffset),
        "Coarser lod offset for grids for use with balanced traversal.")

    ((section + "balancedGridNeighborsDistance").c_str(),
        po::value<uint32>(&opts->balancedGridNeighborsDistance)
        ->default_value(opts->balancedGridNeighborsDistance),
        "Distance to neighbors for grids for use with balanced traversal.")
    ;
}

void optionsConfigNavigation(
    boost::program_options::options_description &desc,
    NavigationOptions *opts,
    std::string section)
{
    sanitizeSection(section);
    desc.add_options()
    FILE_OPTIONS

    ((section + "type").c_str(),
        po::value<NavigationType>(&opts->type)
        ->default_value(opts->type),
        "Responsiveness:\n"
        "instant\n"
        "quick\n"
        "flyOver")

    ((section + "mode").c_str(),
        po::value<NavigationMode>(&opts->mode)
        ->default_value(opts->mode),
        "Behavior:\n"
        "azimuthal\n"
        "free\n"
        "dynamic\n"
        "seamless")

    ((section + "enableNormalization").c_str(),
        po::value<bool>(&opts->enableNormalization)
        ->default_value(opts->enableNormalization)
        ->implicit_value(!opts->enableNormalization),
        "Limits camera tilt and yaw.")

    ((section + "enableAltitudeCorrections").c_str(),
        po::value<bool>(&opts->enableAltitudeCorrections)
        ->default_value(opts->enableAltitudeCorrections)
        ->implicit_value(!opts->enableAltitudeCorrections),
        "Vertically converges objective position towards ground.")
    ;
}

void optionsConfigFetcherOptions(
        boost::program_options::options_description &desc,
        FetcherOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()
    FILE_OPTIONS

    ((section + "threads").c_str(),
        po::value<uint32>(&opts->threads)
        ->default_value(opts->threads),
        "Number of threads created for the fetcher.")

    ((section + "maxHostConnections").c_str(),
        po::value<uint32>(&opts->maxHostConnections)
        ->default_value(opts->maxHostConnections),
        "Maximum concurrent connections to same host (per thread).")

    ((section + "maxTotalConnections").c_str(),
        po::value<uint32>(&opts->maxTotalConnections)
        ->default_value(opts->maxTotalConnections),
        "Total limit of concurrent connections (per thread).")

    ((section + "maxCacheConections").c_str(),
        po::value<uint32>(&opts->maxCacheConections)
        ->default_value(opts->maxCacheConections),
        "Size of curl connection cache (per thread).")

    ((section + "pipelining").c_str(),
        po::value<sint32>(&opts->pipelining)
        ->default_value(opts->pipelining),
        "HTTP pipelining mode.")

    ((section + "extraFileLog").c_str(),
        po::value<bool>(&opts->extraFileLog)
        ->default_value(opts->extraFileLog)
        ->implicit_value(!opts->extraFileLog),
        "Produce separate log with downloads.")
    ;
}

} // namespace vts
