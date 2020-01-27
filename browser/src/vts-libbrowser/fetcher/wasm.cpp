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

#include <stdio.h>
#include <string.h>
#include <emscripten/fetch.h>

#include "dbglog/dbglog.hpp"
#include "../include/vts-browser/fetcher.hpp"
#include "../utilities/threadQueue.hpp"

#include <list>
#include <thread>

namespace vts
{

namespace
{

class WasmTask
{
    std::shared_ptr<FetchTask> task;
    emscripten_fetch_t *fetch;

public:
    WasmTask(const std::shared_ptr<FetchTask> &task)
        : task(task), fetch(nullptr)
    {
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        attr.userData = this;
        strcpy(attr.requestMethod, "GET");
        // todo attr.timeoutMSecs
        // todo attr.requestHeaders
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY
                //| EMSCRIPTEN_FETCH_WAITABLE
                | EMSCRIPTEN_FETCH_SYNCHRONOUS;
        // waitable fetch is not yet implemented in emscripten for WASM
        // use synchronous fetch as a temporary workaround
        fetch = emscripten_fetch(&attr, task->query.url.c_str());
    }

    ~WasmTask()
    {
        task->fetchDone();
        emscripten_fetch_close(fetch);
    }

    WasmTask(const WasmTask &) = delete;
    WasmTask &operator = (const WasmTask &) = delete;

    bool update()
    {
        auto ret = emscripten_fetch_wait(fetch, 0);
        if (ret == EMSCRIPTEN_RESULT_TIMED_OUT)
            return false; // todo handle other error codes
        task->reply.code = fetch->status;
        task->reply.content.allocate(fetch->numBytes);
        memcpy(task->reply.content.data(), fetch->data, fetch->numBytes);
        return true;
    }
};

class FetcherImpl : public Fetcher
{
    //std::list<std::unique_ptr<WasmTask>> tasks;
    ThreadQueue<std::shared_ptr<FetchTask>> que;
    std::vector<std::thread> thrs;

public:
    FetcherImpl(const FetcherOptions &options)
    {
        static const uint32 thrsCnt = 20;
        thrs.reserve(thrsCnt);
        for (uint32 i = 0; i < thrsCnt; i++)
            thrs.push_back(std::thread(&FetcherImpl::thrEntry, this));
    }

    void thrEntry()
    {
        while (true)
        {
            std::shared_ptr<FetchTask> task;
            if (!que.waitPop(task))
                return;
            auto t = std::make_unique<WasmTask>(task);
            while (!t->update()) {}
        }
    }

    void finalize() override
    {
        //tasks.clear();
        que.terminate();
        for (auto &it : thrs)
            it.join();
    }

    void update() override
    {
        /*
        auto it = tasks.begin();
        while (it != tasks.end())
        {
            if ((*it)->update())
                it = tasks.erase(it);
            else
                it++;
        }
        */
    }

    void fetch(const std::shared_ptr<FetchTask> &task) override
    {
        //tasks.insert(tasks.end(), std::make_unique<WasmTask>(task));
        que.push(task);
    }
};

} // namespace

std::shared_ptr<Fetcher> Fetcher::create(const FetcherOptions &options)
{
    return std::dynamic_pointer_cast<Fetcher>(
        std::make_shared<FetcherImpl>(options));
}

} // namespace vts

