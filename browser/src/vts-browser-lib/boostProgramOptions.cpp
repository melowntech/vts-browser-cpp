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

} // namespace

std::istream &operator >> (std::istream &in, TraverseMode &mode)
{
    std::string token;
    in >> token;
    boost::to_lower(token);

    if (token == "hierarchical")
        mode = TraverseMode::Hierarchical;
    else if (token == "flat")
        mode = TraverseMode::Flat;
    else
        throw boost::program_options::validation_error(
                boost::program_options::validation_error::invalid_option_value,
                "map.traverseMode", token);

    return in;
}

void optionsConfigLog(
        boost::program_options::options_description &desc)
{
    desc.add_options()
    ("log.mask",
        po::value<std::string>()->notifier(&notifyLogMask),
        "Set log mask.\n"
        "Format: I?E?W?\n"
        "Eg.: I3W1E4"
    )
    ("log.file",
        po::value<std::string>()->notifier(&notifyLogFile),
        "Set log output file path.")
    ("log.console",
        po::value<bool>()->notifier(&notifyLogConsole),
        "Enable log output to console.")
    ;
}

void optionsConfigCreateOptions(
        boost::program_options::options_description &desc,
        class MapCreateOptions *opts)
{
    desc.add_options()
    ("map.clientId",
        po::value<std::string>(&opts->clientId),
        "Identification of the application for the authentication server.")
    ("map.cachePath",
        po::value<std::string>(&opts->cachePath),
        "Path to a directory where all downloaded resources are cached.")
    ("map.disableCache",
        po::value<bool>(&opts->disableCache),
        "Set to yes to completly disable the cache.")
    ;
}

void optionsConfigMapOptions(
        boost::program_options::options_description &desc,
        class MapOptions *opts)
{
    desc.add_options()
    ("map.maxResourcesMemory",
        po::value<uint64>(&opts->maxResourcesMemory),
        "Maximum memory (in bytes) used by resources "
        "before they begin to unload.")
    ("map.traverseMode",
        po::value<TraverseMode>(&opts->traverseMode),
        "Render traversal mode:\n"
        "hierarchical\n"
        "flat")
    ;
}

void optionsConfigFetcherOptions(
        boost::program_options::options_description &desc,
        class FetcherOptions *opts)
{
    desc.add_options()
    ("fetcher.threads",
        po::value<uint32>(&opts->threads),
        "Number of threads created for the fetcher.")
    ("fetcher.maxHostConnections",
        po::value<uint32>(&opts->maxHostConnections),
        "Maximum concurrent connections to same host.")
    ("fetcher.maxTotalConections",
        po::value<uint32>(&opts->maxTotalConections),
        "Total limit of concurrent connections.")
    ("fetcher.maxCacheConections",
        po::value<uint32>(&opts->maxCacheConections),
        "Size of curl connection cache.")
    ("fetcher.pipelining",
        po::value<sint32>(&opts->pipelining),
        "HTTP pipelining mode.")
    ;
}

} // namespace vts
