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

#include <functional>
#include <unistd.h> // usleep
#include <http/http.hpp>
#include <http/resourcefetcher.hpp>

#include "include/vts-browser/fetcher.hpp"

namespace vts
{

namespace
{

class FetcherImpl;

class Task
{
public:
    Task(FetcherImpl *impl, const std::shared_ptr<FetchTask> &task);
    ~Task();
    void done(http::ResourceFetcher::MultiQuery &&queries);
    void finish();
    
    FetcherImpl *const impl;
    http::ResourceFetcher::Query query;
    std::shared_ptr<FetchTask> task;
    bool called;
};

class FetcherImpl : public Fetcher
{
public:
    FetcherImpl(const FetcherOptions &options) : options(options),
        fetcher(htt.fetcher()), initCount(0), extraLog(nullptr)
    {
        begin = std::chrono::high_resolution_clock::now();
        if (options.extraFileLog)
        {
            extraLog = fopen("downloads.log", "at");
            if (extraLog)
                fprintf(extraLog, "%ld starting download log\n", time());
        }
    }
    
    ~FetcherImpl()
    {
        assert(initCount == 0);
        if (extraLog)
        {
            fprintf(extraLog, "%ld finished download log\n", time());
            fclose(extraLog);
        }
    }
    
    virtual void initialize() override
    {
        if (initCount++ == 0)
        {
            http::ContentFetcher::Options o;
            o.maxCacheConections = options.maxCacheConections;
            o.maxHostConnections = options.maxHostConnections;
            o.maxTotalConections = options.maxTotalConections;
            o.pipelining = options.pipelining;
            htt.startClient(options.threads, &o);
        }
    }
    
    virtual void finalize() override
    {
        if (--initCount == 0)
        {
            htt.stop();
        }
    }

    void fetch(const std::shared_ptr<FetchTask> &task) override
    {
        assert(initCount > 0);
        assert(task->reply.code == 0);
        auto t = std::make_shared<Task>(this, task);
        fetcher.perform(t->query, std::bind(&Task::done, t,
                                            std::placeholders::_1));
        if (extraLog)
            fprintf(extraLog, "%ld init %s\n", time(), task->query.url.c_str());
    }

    uint64 time()
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - begin).count();
    }

    const FetcherOptions options;
    http::Http htt;
    http::ResourceFetcher fetcher;
    std::atomic<int> initCount;
    FILE *extraLog;
    std::chrono::high_resolution_clock::time_point begin;
};

Task::Task(FetcherImpl *impl, const std::shared_ptr<FetchTask> &task)
    : impl(impl), query(task->query.url), task(task), called(false)
{
    query.timeout(impl->options.timeout);
    for (auto it : task->query.headers)
        query.addOption(it.first, it.second);
}

Task::~Task()
{
    try // destructor must not throw
    {
        if (!called)
            finish();
    }
    catch(...)
    {
        // do nothing
    }
}

void Task::done(utility::ResourceFetcher::MultiQuery &&queries)
{
    assert(queries.size() == 1);
    assert(task->reply.code == 0);
    assert(!called);
    called = true;
    http::ResourceFetcher::Query &q = *queries.begin();
    if (q.valid())
    {
        const http::ResourceFetcher::Query::Body &body = q.get();
        if (body.redirect)
        {
            task->reply.code = body.redirect.value();
        }
        else
        {
            task->reply.content.allocate(body.data.size());
            memcpy(task->reply.content.data(), body.data.data(),
                   body.data.size());
            task->reply.contentType = body.contentType;
            task->reply.expires = body.expires;
            task->reply.code = 200;

            // testing start
            //if (task->resourceType == FetchTask::ResourceType::MetaTile)
            //    task->replyCode = FetchTask::ExtraCodes::SimulatedError;
            // testing end
        }
    }
    else if (q.ec())
    {
        task->reply.code = q.ec().value();
    }
    else if (q.exc())
    {
        try
        {
            std::rethrow_exception(q.exc());
        }
        catch (std::error_code &e)
        {
            task->reply.code = e.value();
        }
        catch (std::exception &e)
        {
            LOG(err1) << "Exception <" << e.what()
                      << "> in download of <" << task->query.url << ">";
            task->reply.code = FetchTask::ExtraCodes::InternalError;
        }
        catch (...)
        {
            LOG(err1) << "Unknown exception in download of <"
                      << task->query.url << ">";
            task->reply.code = FetchTask::ExtraCodes::InternalError;
        }
    }
    else
    {
        LOG(err1) << "Invalid result from HTTP fetcher (no value, no status "
            "code, no exception) for <" << task->query.url << ">";
        task->reply.code = FetchTask::ExtraCodes::InternalError;
    }
    finish();
}

void Task::finish()
{
    if (impl->extraLog)
    {
        fprintf(impl->extraLog, "%ld done %d %d %s %s\n",
                impl->time(), task->reply.code, task->reply.content.size(),
                task->query.url.c_str(), task->reply.contentType.c_str());
    }
    task->fetchDone();
}

} // namespace

FetcherOptions::FetcherOptions()
    : threads(1),
      timeout(30000),
      extraFileLog(false),
      maxHostConnections(0),
      maxTotalConections(3),
      maxCacheConections(0),
      pipelining(2)
{}

Fetcher::~Fetcher()
{}

std::shared_ptr<Fetcher> Fetcher::create(const FetcherOptions &options)
{
    return std::dynamic_pointer_cast<Fetcher>(
                std::make_shared<FetcherImpl>(options));
}

FetchTask::Query::Query(const std::string &url,
                        FetchTask::ResourceType resourceType) :
    url(url), resourceType(resourceType)
{}

FetchTask::Reply::Reply() : expires(-1), code(0)
{}

FetchTask::FetchTask(const std::string &url, ResourceType resourceType) :
    query(url, resourceType)
{}

FetchTask::~FetchTask()
{}

} // namespace vts
