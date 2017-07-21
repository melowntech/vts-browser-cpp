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

#include <clocale>
#include <boost/program_options.hpp>
#include <QGuiApplication>
#include <vts-browser/map.hpp>
#include <vts-browser/options.hpp>
#include "mainWindow.hpp"
#include "gpuContext.hpp"

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    std::string config, auth, sri;

    // parse command line arguments
    {
        po::options_description desc("Options");
        desc.add_options()
                ("help", "Show this help.")
                ("url,u",
                    po::value<std::string>(&config),
                    "Mapconfig url."
                )
                ("auth,a",
                    po::value<std::string>(&auth),
                    "Auth url."
                )
                ("sri,s",
                    po::value<std::string>(&sri),
                    "SRI url."
                );

        po::positional_options_description popts;
        popts.add("url", 1);

        po::variables_map vm;

        try
        {
            po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(popts).run(), vm);
            po::notify(vm);
        }
        catch (const std::exception &e)
        {
            std::cout << e.what() << std::endl;
            return 2;
        }

        if (vm.count("help"))
        {
            std::cout << "Usage: " << argv[0] << " [options] [--]"
                      << " <mapconfig-url>"
                      << std::endl << desc << std::endl;
            return 1;
        }
    }

    if (config.empty())
    {
        std::cout << "Mapconfig url must be provided." << std::endl;
        return 2;
    }

    // create the Qt application
    QGuiApplication application(argc, argv);

    // the QGuiApplication changed the locale based on environment
    // we need to revert it back
    std::setlocale(LC_ALL, "C");

    // create the window
    MainWindow mainWindow;
    mainWindow.resize(QSize(800, 600));
    mainWindow.show();

    // create the map
    vts::MapCreateOptions options;
    options.clientId = "vts-browser-qt";
    vts::Map map(options);
    map.setWindowSize(mainWindow.size().width(), mainWindow.size().height());
    map.setMapConfigPath(config, auth, sri);

    // initialize the window with the map
    mainWindow.map = &map;
    mainWindow.initialize();
    mainWindow.requestUpdate();

    // run the main event loop
    return application.exec();
}
