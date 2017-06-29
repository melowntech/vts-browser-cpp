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

#include <unistd.h> // usleep
#include <vts-browser/map.hpp>
#include <vts-browser/log.hpp>
#include "dataThread.hpp"
#include "gpuContext.hpp"
#include <GLFW/glfw3.h>

namespace
{
    void run(DataThread *data)
    {
        data->run();
    }
}

DataThread::DataThread(GLFWwindow *shared, double *timing) : map(nullptr),
    window(nullptr), timing(timing), stop(false)
{
    vts::FetcherOptions options;
    fetcher = vts::Fetcher::create(options);
    glfwWindowHint(GLFW_VISIBLE, false);
    window = glfwCreateWindow(1, 1, "data context", NULL, shared);
    glfwSetWindowUserPointer(window, this);
    glfwHideWindow(window);
    initializeGpuContext();
    thr = std::thread(&::run, this);
}

DataThread::~DataThread()
{
    stop = true;
    thr.join();
    glfwDestroyWindow(window);
    window = nullptr;
}

void DataThread::run()
{
    vts::setLogThreadName("data");
    glfwMakeContextCurrent(window);
    while (!stop && !map)
        usleep(1000);
    map->dataInitialize(fetcher);
    while (!stop)
    {
        double timeFrameStart = glfwGetTime();
        bool a = map->dataTick();
        double timeFrameEnd = glfwGetTime();
        *timing = timeFrameEnd - timeFrameStart;
        if (a)
            usleep(20000);
        else
            usleep(5000);
    }
    map->dataFinalize();
}
