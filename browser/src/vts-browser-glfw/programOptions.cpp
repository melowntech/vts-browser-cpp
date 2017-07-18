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

MainWindow::Paths parseConfigPaths(std::string config)
{
    assert(!config.empty());
    std::vector<std::string> a;
    boost::split(a, config, boost::is_any_of("|"));
    if (a.size() > 3)
        throw std::runtime_error("Config path contains too many parts.");
    MainWindow::Paths r;
    r.mapConfig = a[0];
    if (a.size() > 1)
        r.auth = a[1];
    if (a.size() > 2)
        r.sri = a[2];
    return r;
}

bool programOptions(vts::MapCreateOptions &createOptions,
                    vts::MapOptions &mapOptions,
                    vts::FetcherOptions &fetcherOptions,
                    std::vector<MainWindow::Paths> &paths,
                    int argc, char *argv[])
{
    std::vector<std::string> configs;

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
            );

    po::positional_options_description popts;
    popts.add("url", -1);

    vts::optionsConfigLog(desc);
    vts::optionsConfigCreateOptions(desc, &createOptions);
    vts::optionsConfigMapOptions(desc, &mapOptions);
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
        throw std::runtime_error("At least one mapconfig is required.");

    for (auto &&it : configs)
        paths.push_back(parseConfigPaths(it));

    return true;
}
