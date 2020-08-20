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

#include <GLFW/glfw3.h>
#include <optick.h>

#include "mainWindow.hpp"
#include "dataThread.hpp"
#include "programOptions.hpp"

#include <vts-renderer/highPerformanceGpuHint.h>

void logGlfwErr()
{
#if GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR > 3003
    const char *errStr = nullptr;
    glfwGetError(&errStr);
    vts::log(vts::LogLevel::err4, errStr);
#endif
}

void initializeGlfw(GLFWwindow *&renderWindow, GLFWwindow *&dataWindow)
{
    if (glfwInit() != GLFW_TRUE)
    {
        logGlfwErr();
        throw std::runtime_error("Failed to initialize GLFW");
    }

    {
        glfwWindowHint(GLFW_VISIBLE, 0);
        glfwWindowHint(GLFW_RESIZABLE, 1);
        glfwWindowHint(GLFW_MAXIMIZED, 1);
        glfwWindowHint(GLFW_RED_BITS, 8);
        glfwWindowHint(GLFW_GREEN_BITS, 8);
        glfwWindowHint(GLFW_BLUE_BITS, 8);
        glfwWindowHint(GLFW_DEPTH_BITS, 0);
        glfwWindowHint(GLFW_ALPHA_BITS, 0);
        glfwWindowHint(GLFW_STENCIL_BITS, 0);
        glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 0);
#ifdef NDEBUG
        glfwWindowHint(GLFW_CONTEXT_NO_ERROR, 1);
#else
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif
        renderWindow = glfwCreateWindow(800, 600, "vts-browser-desktop", nullptr, nullptr);
    }

    if (!renderWindow)
    {
        logGlfwErr();
        throw std::runtime_error("Failed to create opengl window");
    }

    dataWindow = glfwCreateWindow(10, 10, "", nullptr, renderWindow);
    if (!dataWindow)
    {
        logGlfwErr();
        throw std::runtime_error("Failed to create shared opengl context");
    }

    glfwMakeContextCurrent(renderWindow);
    glfwSwapInterval(1);
}

void finalizeGlfw(GLFWwindow *renderWindow, GLFWwindow *dataWindow)
{
    glfwDestroyWindow(renderWindow);
    glfwDestroyWindow(dataWindow);
    glfwTerminate();
}

int main(int argc, char *argv[])
{
    // release build -> catch exceptions and print them to stderr
    // debug build -> let the debugger handle the exceptions
#ifdef NDEBUG
    try
    {
#endif
        OPTICK_THREAD("main");
        vts::setLogThreadName("main");
        vts::setLogFile("vts-browser-desktop.log");
        //vts::setLogMask("I2W2E2");
        //vts::setLogMask("D");

        vts::MapCreateOptions createOptions;
        createOptions.clientId = "vts-browser-desktop";
        vts::MapRuntimeOptions mapOptions;
        vts::CameraOptions camOptions;
        vts::NavigationOptions navOptions;
        mapOptions.targetResourcesMemoryKB = 512 * 1024;
        vts::FetcherOptions fetcherOptions;
        AppOptions appOptions;
        vts::renderer::RenderOptions renderOptions;
        renderOptions.colorToTexture = true;
        if (!programOptions(createOptions, mapOptions, fetcherOptions, camOptions, navOptions, renderOptions, appOptions, argc, argv))
            return 0;
        struct GLFWwindow *renderWindow = nullptr;
        struct GLFWwindow *dataWindow = nullptr;
        initializeGlfw(renderWindow, dataWindow);
        vts::renderer::loadGlFunctions((GLADloadproc)&glfwGetProcAddress);

        // vts map and main loop entry
        {
            vts::Map map(createOptions, vts::Fetcher::create(fetcherOptions));
            auto camera = map.createCamera();
            auto navigation = camera->createNavigation();
            map.options() = mapOptions;
            camera->options() = camOptions;
            navigation->options() = navOptions;
            DataThread data(dataWindow, &map);
            MainWindow main(renderWindow, &map, camera.get(), navigation.get(), appOptions, renderOptions);
            data.start();
            main.run();
        }

        finalizeGlfw(renderWindow, dataWindow);
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

