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
#include "mainWindow.hpp"
#include "dataThread.hpp"
#include "threadName.hpp"
#include <GLFW/glfw3.h>

void errorCallback(int, const char* description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
}

void usage(char *argv[])
{
    printf("Usage: %s [options] [--] <config-url> [config-url]...\n", argv[0]);
    printf("Options:\n");
    printf("\t--auth=<url>\n\t\tAuthentication url.\n");
}

int main(int argc, char *argv[])
{
    // release build -> catch exceptions and print them to stderr
    // debug build -> let the debugger handle the exceptions
#ifdef NDEBUG
    try
    {
#endif
        //vts::setLogMask("ND");
        //vts::setLogMask("ALL");
        
        const char *auth = "";
        int firstUrl = argc;
        
        for (int i = 1; i < argc; i++)
        {
            // --
            if (argv[i][0] != '-')
            {
                firstUrl = i;
                break;
            }
            if (strcmp(argv[i], "--") == 0)
            {
                firstUrl = i + 1;
                break;
            }
            
            // --auth=
            if (strncmp(argv[i], "--auth=", 7) == 0)
            {
                const char *a = argv[i] + 7;
                if (a[0] == 0)
                {
                    fprintf(stderr, "Missing auth url.\n");
                    usage(argv);
                    return 4;
                }
                auth = a;
                continue;
            }
            
            fprintf(stderr, "Unknown option '%s'\n", argv[i]);
            usage(argv);
            return 4;
        }
        if (firstUrl >= argc)
        {
            usage(argv);
            return 3;
        }
    
        glfwSetErrorCallback(&errorCallback);
        if (!glfwInit())
            return 2;
    
        {
            vts::MapCreateOptions options("vts-browser-glfw");
            vts::Map map(options);
            MainWindow main;
            for (int i = firstUrl; i < argc; i++)
                main.mapConfigPaths.push_back(argv[i]);
            main.authPath = auth;
            map.setMapConfigPath(argv[firstUrl], auth);
            DataThread data(main.window, &main.timingDataFrame);
            main.map = &map;
            data.map = &map;
            setThreadName("main");
            main.run();
        }

        glfwTerminate();
        return 0;
#ifdef NDEBUG
    }
    catch(const std::exception &e)
    {
        fprintf(stderr, "Exception: %s\n", e.what());
        return 1;
    }
    catch(const char *e)
    {
        fprintf(stderr, "Exception: %s\n", e);
        return 1;
    }
    catch(...)
    {
        fprintf(stderr, "Unknown exception.\n");
        return 1;
    }
#endif
}
