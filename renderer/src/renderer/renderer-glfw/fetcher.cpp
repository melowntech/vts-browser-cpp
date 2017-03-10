#include <functional>
#include <unistd.h> // usleep

#include <http/http.hpp>
#include <http/resourcefetcher.hpp>
#include <http/sink.hpp>

#include "fetcher.h"

namespace
{

class FetcherImpl;

class Task
{
public:
    Task(melown::FetchTask *task, melown::Fetcher::Func func)
        : query(task->url), task(task), func(func)
    {
        query.timeout(-1);
    }
    
    void done(http::ResourceFetcher::MultiQuery &&queries)
    {
        task->code = 404;
        http::ResourceFetcher::Query &q = *queries.begin();
        if (q.valid())
        {
            const http::ResourceFetcher::Query::Body &body = q.get();
            task->contentData.allocate(body.data.size());
            memcpy(task->contentData.data, body.data.data(), body.data.size());
            task->contentType = body.contentType;
            task->code = 200;
        }
        try
        {
            func(task);
        }
        catch (...)
        {}
        delete this;
    }
    
    http::ResourceFetcher::Query query;
    melown::Fetcher::Func func;
    melown::FetchTask *task;
};

class FetcherImpl : public Fetcher
{
public:
    FetcherImpl() : fetcher(htt.fetcher())
    {}

    ~FetcherImpl()
    {}

    void initialize(melown::Fetcher::Func func) override
    {
        this->func = func;
        htt.startClient(4);
    }
    
    void finalize() override
    {
        htt.stop();
    }

    void fetch(melown::FetchTask *task) override
    {
        Task *t = new Task(task, func);
        fetcher.perform(t->query, std::bind(&Task::done, t,
                                            std::placeholders::_1));
    }

    void tick() override
    {
        // do nothing
    }

    melown::Fetcher::Func func;
    http::Http htt;
    http::ResourceFetcher fetcher;
};

} // namespace

Fetcher *Fetcher::create()
{
    return new FetcherImpl();
}
