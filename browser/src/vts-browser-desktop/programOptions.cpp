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

#include "programOptions.hpp"
#include <vts-browser/boostProgramOptions.hpp>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options;

MapPaths parseConfigPaths(const std::string &config,
                                   const std::string &auth,
                                   const std::string &sri)
{
    assert(!config.empty());
    std::vector<std::string> a;
    boost::split(a, config, boost::is_any_of("|"));
    if (a.size() > 3)
        throw std::runtime_error("Config path contains too many parts.");
    MapPaths r;
    r.mapConfig = a[0];
    if (a.size() > 1)
        r.auth = a[1];
    else
        r.auth = auth;
    if (a.size() > 2)
        r.sri = a[2];
    else
        r.sri = sri;
    return r;
}

bool programOptions(vts::MapCreateOptions &createOptions,
                    vts::MapOptions &mapOptions,
                    vts::FetcherOptions &fetcherOptions,
                    AppOptions &appOptions,
                    int argc, char *argv[])
{
    std::vector<std::string> configs;
    std::string auth, sri;

    po::options_description desc("Options");
    desc.add_options()
            ("help", "Show this help.")
            ("url",
                po::value<std::vector<std::string>>(&configs)->composing(),
                "Mapconfig URL(s).\n"
                "Available formats:\n"
                "<config>|<auth>|<sri>\n"
                "<config>|<auth>\n"
                "<config>||<sri>\n"
                "<config>"
            )
            ("auth,a",
                po::value<std::string>(&auth),
                "Authentication url fallback."
            )
            ("sri,s",
                po::value<std::string>(&sri),
                "SRI url fallback."
            )
            ("position,p",
                po::value<std::string>(&appOptions.initialPosition),
                "Override initial position for the first map.\n"
                "Uses url format, eg.:\n"
                "obj,long,lat,fix,height,pitch,yaw,roll,extent,fov"
            )
            ("closeWhenComplete",
                po::value<bool>(&appOptions.closeOnFullRender)
                ->default_value(appOptions.closeOnFullRender)
                ->implicit_value(!appOptions.closeOnFullRender),
                "Quit the application when it finishes "
                "rendering the whole image."
            )
            ("purgeCache",
                po::value<bool>(&appOptions.purgeDiskCache)
                ->default_value(appOptions.purgeDiskCache)
                ->implicit_value(!appOptions.purgeDiskCache),
                "Purge the disk cache during the initialization."
            )
            ("renderAtmosphere",
                po::value<bool>(&appOptions.render.renderAtmosphere)
                ->default_value(appOptions.render.renderAtmosphere)
                ->implicit_value(!appOptions.render.renderAtmosphere),
                "Render atmosphere."
            )
            ("antialiasing",
                po::value<int>(&appOptions.render.antialiasingSamples)
                ->default_value(appOptions.render.antialiasingSamples)
                ->implicit_value(16),
                "Antialiasing samples."
            )
            /*("screenshotWhenComplete",
                po::value<bool>(&appOptions.screenshotOnFullRender)
             ->default_value(appOptions.screenshotOnFullRender)
             ->implicit_value(!appOptions.screenshotOnFullRender),
                "Save screenshot when it finishes "
                "rendering the whole image."
            )*/
            ;

    po::positional_options_description popts;
    popts.add("url", -1);

    vts::optionsConfigLog(desc);
    vts::optionsConfigCreateOptions(desc, &createOptions);
    vts::optionsConfigMapOptions(desc, &mapOptions);
    vts::optionsConfigDebugOptions(desc, &mapOptions);
    vts::optionsConfigFetcherOptions(desc, &fetcherOptions);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
          options(desc).positional(popts).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << "Usage: " << argv[0] << " [options] [--]"
                  << " <urls> [urls ...]"
                  << std::endl << desc << std::endl;
        return false;
    }

    if (configs.empty())
        throw std::runtime_error("At least one mapconfig url is required.");

    for (auto &&it : configs)
        appOptions.paths.push_back(parseConfigPaths(it, auth, sri));

    return true;
}
