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

#include "../include/vts-browser/fetcher.hpp"

namespace vts
{

namespace
{

class WasmTask
{
public:
    std::shared_ptr<class FetcherImpl> fetcher;
    std::shared_ptr<FetchTask> task;

    void done(emscripten_fetch_t *fetch)
    {
        task->reply.code = fetch->status;
        task->reply.content.allocate(fetch->numBytes);
        memcpy(task->reply.content.data(), fetch->data, fetch->numBytes);
    }
};

void downloadDone(emscripten_fetch_t *fetch)
{
    WasmTask *wt = (WasmTask*)fetch->userData;
    try
    {
        wt->done(fetch);
    }
    catch (...)
    {
        // dont care
    }
    emscripten_fetch_close(fetch);
    wt->task->fetchDone();
    delete wt;
}

class FetcherImpl : public Fetcher, std::enable_shared_from_this<FetcherImpl>
{
public:
    FetcherImpl(const FetcherOptions &options)
    {}

    ~FetcherImpl()
    {}

    void initialize() override
    {}

    void finalize() override
    {}

    void fetch(const std::shared_ptr<FetchTask> &task) override
    {
        try
        {
            WasmTask *wt = new WasmTask();
            wt->fetcher = shared_from_this();
            wt->task = task;
            emscripten_fetch_attr_t attr;
            emscripten_fetch_attr_init(&attr);
            attr.userData = wt;
            strcpy(attr.requestMethod, "GET");
            // todo attr.timeoutMSecs
            // todo attr.requestHeaders
            attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
            attr.onsuccess = downloadDone;
            attr.onerror = downloadDone;
            emscripten_fetch(&attr, task->query.url.c_str());
        }
        catch (...)
        {
            task->reply.code = FetchTask::ExtraCodes::InternalError;
            task->fetchDone();
        }
    }
};

} // namespace

std::shared_ptr<Fetcher> Fetcher::create(const FetcherOptions &options)
{
    return std::dynamic_pointer_cast<Fetcher>(
        std::make_shared<FetcherImpl>(options));
}

} // namespace vts

