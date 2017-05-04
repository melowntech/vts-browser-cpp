#include <functional>
#include <unistd.h> // usleep

#include <http/http.hpp>
#include <http/resourcefetcher.hpp>
#include <http/sink.hpp>

#include <vts/fetcher.hpp>

namespace vts
{

namespace
{

class FetcherImpl;

class Task
{
public:
    Task(std::shared_ptr<vts::FetchTask> task,
         vts::Fetcher::Func func, sint32 timeout)
        : query(task->url), task(task), func(func)
    {
        query.timeout(timeout);
    }
    
    void done(http::ResourceFetcher::MultiQuery &&queries)
    {
        http::ResourceFetcher::Query &q = *queries.begin();
        if (q.valid())
        {
            const http::ResourceFetcher::Query::Body &body = q.get();
            task->contentData.allocate(body.data.size());
            memcpy(task->contentData.data(), body.data.data(),
                   body.data.size());
            task->contentType = body.contentType;
            task->code = 200;
        }
        else
            task->code = q.ec().value();
        try
        {
            func(task);
        }
        catch (...)
        {
			// do nothing
		}
        delete this;
    }
    
    http::ResourceFetcher::Query query;
    vts::Fetcher::Func func;
    std::shared_ptr<vts::FetchTask> task;
};

class FetcherImpl : public Fetcher
{
public:
    FetcherImpl(uint32 threads, sint32 timeout) : fetcher(htt.fetcher()),
        threads(threads), timeout(timeout)
    {}

    ~FetcherImpl()
    {}

    void initialize(vts::Fetcher::Func func) override
    {
        this->func = func;
        htt.startClient(threads);
    }
    
    void finalize() override
    {
        htt.stop();
    }

    void fetch(std::shared_ptr<vts::FetchTask> task) override
    {
        Task *t = new Task(task, func, timeout);
        fetcher.perform(t->query, std::bind(&Task::done, t,
                                            std::placeholders::_1));
    }

    vts::Fetcher::Func func;
    http::Http htt;
    http::ResourceFetcher fetcher;
    uint32 threads;
    sint32 timeout;
};

} // namespace

Fetcher *Fetcher::create(uint32 threads, sint32 timeout)
{
    return new FetcherImpl(threads, timeout);
}

} // namespace vts

