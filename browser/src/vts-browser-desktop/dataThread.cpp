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
#include <vts-browser/fetcher.hpp>
#include <SDL2/SDL.h>
#include "dataThread.hpp"

namespace
{
    void run(DataThread *data)
    {
        data->run();
    }
}

DataThread::DataThread(vts::Map *map, uint32 &timing,
                       SDL_Window *window, void *context,
                       const vts::FetcherOptions &fetcherOptions) :
    map(map), timing(timing),
    window(window), context(context),
    stop(false)
{
    fetcher = vts::Fetcher::create(fetcherOptions);
    thr = std::thread(&::run, this);
}

DataThread::~DataThread()
{
    stop = true;
    thr.join();
}

void DataThread::run()
{
    vts::setLogThreadName("fetcher");
    map->dataInitialize(fetcher);

    vts::setLogThreadName("data");
    SDL_GL_MakeCurrent(window, context);
    while (!stop && !map)
        usleep(10000);
    while (!stop)
    {
        uint32 timeFrameStart = SDL_GetTicks();
        map->dataTick();
        uint32 timeFrameEnd = SDL_GetTicks();
        timing = timeFrameEnd - timeFrameStart;
        usleep(100000);
    }
    map->dataFinalize();
    SDL_GL_DeleteContext(context);
}
