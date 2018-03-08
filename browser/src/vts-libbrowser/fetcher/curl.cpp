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

#include <fstream>
#include <unistd.h> // usleep
#include <http/http.hpp>
#include <http/resourcefetcher.hpp>
#include "../include/vts-browser/fetcher.hpp"

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

    const uint64 begin;
    FetcherImpl *const impl;
    const uint32 id;
    http::ResourceFetcher::Query query;
    std::shared_ptr<FetchTask> task;
    bool called;
};

class FetcherImpl : public Fetcher
{
public:
    FetcherImpl(const FetcherOptions &options) : options(options),
        fetcher(htt.fetcher()), initCount(0), taskId(0), extraLog(nullptr)
    {
        begin = std::chrono::high_resolution_clock::now();
        if (options.extraFileLog)
        {
            extraLog.open("downloads.log");
            if (extraLog)
                extraLog << time() << " starting download log" << std::endl;
        }
    }

    ~FetcherImpl()
    {
        assert(initCount == 0);
        if (extraLog)
            extraLog << time() << " finished download log" << std::endl;
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
        {
            extraLog << t->begin << " init " << t->id
                     << " " << task->query.url << std::endl;
        }
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
    std::atomic<uint32> taskId;
    std::ofstream extraLog;
    std::chrono::high_resolution_clock::time_point begin;
};

Task::Task(FetcherImpl *impl, const std::shared_ptr<FetchTask> &task)
    : begin(impl->time()), impl(impl), id(impl->taskId++),
      query(task->query.url), task(task), called(false)
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
            //if (task->query.resourceType == FetchTask::ResourceType::Mesh
            //    && (std::hash<std::string>()(task->query.url) % 13) == 0)
            //    task->reply.code = FetchTask::ExtraCodes::SimulatedError;
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
            LOG(err2) << "Exception <" << e.what()
                      << "> in download of <" << task->query.url << ">";
            task->reply.code = FetchTask::ExtraCodes::InternalError;
        }
        catch (...)
        {
            LOG(err2) << "Unknown exception in download of <"
                      << task->query.url << ">";
            task->reply.code = FetchTask::ExtraCodes::InternalError;
        }
    }
    else
    {
        LOG(err3) << "Invalid result from HTTP fetcher (no value, no status "
            "code, no exception) for <" << task->query.url << ">";
        task->reply.code = FetchTask::ExtraCodes::InternalError;
    }
    finish();
}

void Task::finish()
{
    assert(!called);
    called = true;
    if (impl->extraLog)
    {
        impl->extraLog << 
            impl->time() << " done " << id << " " << (impl->time() - begin)
            << " " << task->reply.code << " " << task->reply.content.size()
            << " " << task->reply.contentType << std::endl;
    }
    task->fetchDone();
}

} // namespace

std::shared_ptr<Fetcher> Fetcher::create(const FetcherOptions &options)
{
    return std::dynamic_pointer_cast<Fetcher>(
                std::make_shared<FetcherImpl>(options));
}

} // namespace vts
