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

#include <cstdio>
#include <cstdlib>
#include <vts-browser/map.hpp>
#include <vts-browser/options.hpp>
#include <vts-browser/log.hpp>
#include "mainWindow.hpp"
#include "dataThread.hpp"
#include "programOptions.hpp"
#include <GLFW/glfw3.h>

void errorCallback(int, const char* description)
{
    std::stringstream s;
    s << "GLFW error <" << description << ">";
    vts::log(vts::LogLevel::err4, s.str());
}

int main(int argc, char *argv[])
{
    // release build -> catch exceptions and print them to stderr
    // debug build -> let the debugger handle the exceptions
#ifdef NDEBUG
    try
    {
#endif
        vts::setLogThreadName("main");

        vts::MapCreateOptions createOptions;
        createOptions.clientId = "vts-browser-glfw";
        vts::MapOptions mapOptions;
        vts::FetcherOptions fetcherOptions;
        std::vector<MainWindow::Paths> paths;
        if (!programOptions(createOptions, mapOptions, fetcherOptions, paths,
                            argc, argv))
            return 3;

        glfwSetErrorCallback(&errorCallback);
        if (!glfwInit())
            return 2;

        {
            vts::Map map(createOptions);
            map.options() = mapOptions;
            MainWindow main(&map);
            std::swap(main.paths, paths);
            DataThread data(&map, main.window, &main.timingDataFrame,
                            fetcherOptions);
            main.run();
        }

        glfwTerminate();
        return 0;
#ifdef NDEBUG
    }
    catch(const std::exception &e)
    {
        std::stringstream s;
        s << "Exception <" << e.what() << ">";
        vts::log(vts::LogLevel::err4, s.str());
        return 1;
    }
    catch(const char *e)
    {
        std::stringstream s;
        s << "Exception <" << e << ">";
        vts::log(vts::LogLevel::err4, s.str());
        return 1;
    }
    catch(...)
    {
        vts::log(vts::LogLevel::err4, "Unknown exception.");
        return 1;
    }
#endif
}
