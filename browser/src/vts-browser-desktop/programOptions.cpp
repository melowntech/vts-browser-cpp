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

MapPaths parseConfigPaths(const std::string &config, const std::string &auth)
{
    assert(!config.empty());
    std::vector<std::string> a;
    boost::split(a, config, boost::is_any_of("|"));
    if (a.size() > 2)
        throw std::runtime_error("Config path contains too many parts.");
    MapPaths r;
    r.mapConfig = a[0];
    if (a.size() > 1)
        r.auth = a[1];
    else
        r.auth = auth;
    return r;
}

bool programOptions(vts::MapCreateOptions &createOptions,
                    vts::MapRuntimeOptions &mapOptions,
                    vts::FetcherOptions &fetcherOptions,
                    vts::CameraOptions &camOptions,
                    vts::NavigationOptions &navOptions,
                    vts::renderer::RenderOptions &renderOptions,
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
            "Format: <config>[|<auth>]"
        )
        ("auth",
            po::value<std::string>(&auth),
            "Authentication url fallback."
        )
        ("position,p",
            po::value<std::string>(&appOptions.initialPosition),
            "Override initial position for the first map.\n"
            "Uses url format, eg.:\n"
            "obj,long,lat,fix,height,pitch,yaw,roll,extent,fov"
        )
        ("view,v",
            po::value<std::string>(&appOptions.initialView),
            "Override initial view for the first map.\n"
            "Uses url format."
        )
        ("purgeCache",
            po::value<bool>(&appOptions.purgeDiskCache)
            ->default_value(appOptions.purgeDiskCache)
            ->implicit_value(!appOptions.purgeDiskCache),
            "Purge the disk cache during initialization."
        )
        ("screenshotWhenComplete",
            po::value<bool>(&appOptions.screenshotOnFullRender)
            ->default_value(appOptions.screenshotOnFullRender)
            ->implicit_value(!appOptions.screenshotOnFullRender),
            "Save screenshot when it finishes rendering."
        )
        ("closeWhenComplete",
            po::value<bool>(&appOptions.closeOnFullRender)
            ->default_value(appOptions.closeOnFullRender)
            ->implicit_value(!appOptions.closeOnFullRender),
            "Quit the application when it finishes rendering."
        )
        ("render.atmosphere",
            po::value<bool>(&renderOptions.renderAtmosphere)
            ->default_value(renderOptions.renderAtmosphere)
            ->implicit_value(!renderOptions.renderAtmosphere),
            "Render atmosphere."
        )
        ("render.antialiasing",
            po::value<uint32>(&renderOptions.antialiasingSamples)
            ->default_value(renderOptions.antialiasingSamples)
            ->implicit_value(16),
            "Antialiasing samples count."
        )
        ("render.oversample",
            po::value<uint32>(&appOptions.oversampleRender)
            ->default_value(appOptions.oversampleRender)
            ->implicit_value(2),
            "Rendering resolution multiplier."
        )
        ("gui.scale",
            po::value<double>(&appOptions.guiScale)
            ->default_value(appOptions.guiScale)
            ->implicit_value(2),
            "Gui scale multiplier."
        )
        ("gui.visible",
            po::value<bool>(&appOptions.guiVisible)
            ->default_value(appOptions.guiVisible)
            ->implicit_value(!appOptions.guiVisible),
            "Gui visibility."
        )
        ;

    po::positional_options_description popts;
    popts.add("url", -1);

    vts::optionsConfigLog(desc);
    vts::optionsConfigMapCreate(desc, &createOptions);
    vts::optionsConfigMapRuntime(desc, &mapOptions);
    vts::optionsConfigCamera(desc, &camOptions);
    vts::optionsConfigNavigation(desc, &navOptions);
    vts::optionsConfigFetcherOptions(desc, &fetcherOptions);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(popts).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << "Usage: " << argv[0] << " [options] [--]" << " [urls...]" << std::endl << desc << std::endl;
        return false;
    }

    if (configs.empty())
    {
        MapPaths p;
        p.mapConfig = "https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json";
        appOptions.paths.push_back(p);
    }
    else
    {
        for (auto &it : configs)
            appOptions.paths.push_back(parseConfigPaths(it, auth));
    }

    return true;
}
