#include <functional>
#include <unistd.h> // usleep

#include <http/http.hpp>
#include <http/resourcefetcher.hpp>

#include <vts/fetcher.hpp>

namespace vts
{

namespace
{

class FetcherImpl;

class Task
{
public:
    Task(FetcherImpl *impl, std::shared_ptr<FetchTask> task);
    ~Task();
    void done(http::ResourceFetcher::MultiQuery &&queries);
    
    FetcherImpl *const impl;
    http::ResourceFetcher::Query query;
    std::shared_ptr<FetchTask> task;
    bool called;
};

class FetcherImpl : public Fetcher
{
public:
    FetcherImpl(const FetcherOptions &options) : fetcher(htt.fetcher()),
        options(options)
    {}

    ~FetcherImpl()
    {}

    void initialize(Fetcher::Func func) override
    {
        LOG(info2) << "Initializing fetcher";
        this->func = func;
        http::ContentFetcher::Options o;
        o.maxCacheConections = options.maxCacheConections;
        o.maxHostConnections = options.maxHostConnections;
        o.maxTotalConections = options.maxTotalConections;
        o.pipelining = options.pipelining;
        htt.startClient(options.threads, &o);
    }
    
    void finalize() override
    {
        LOG(info2) << "Finalizing fetcher";
        htt.stop();
    }

    void fetch(std::shared_ptr<FetchTask> task) override
    {
        task->code = 0;
        auto t = std::make_shared<Task>(this, task);
        fetcher.perform(t->query, std::bind(&Task::done, t,
                                            std::placeholders::_1));
    }

    const FetcherOptions options;
    vts::Fetcher::Func func;
    http::Http htt;
    http::ResourceFetcher fetcher;
};

Task::Task(FetcherImpl *impl, std::shared_ptr<FetchTask> task)
    : impl(impl), query(task->url), task(task), called(false)
{
    query.timeout(impl->options.timeout);
}

Task::~Task()
{
    try // destructor must not throw
    {
        if (!called)
            impl->func(task);
    }
    catch(...)
    {
        // do nothing
    }
}

void Task::done(utility::ResourceFetcher::MultiQuery &&queries)
{
    assert(queries.size() == 1);
    assert(task->code == 0);
    assert(!called);
    called = true;
    http::ResourceFetcher::Query &q = *queries.begin();
    task->code = q.ec().value();
    if (q.exc())
    {
        try
        {
            std::rethrow_exception(q.exc());
        }
        catch (std::exception &e)
        {
            LOG(err2) << "Exception <" <<  e.what()
                      << "> in download of '" << task->url << "'";
        }
        catch (...)
        {
            // do nothing
        }
    }
    else if (q.valid())
    {
        const http::ResourceFetcher::Query::Body &body = q.get();
        task->contentData.allocate(body.data.size());
        memcpy(task->contentData.data(), body.data.data(),
               body.data.size());
        task->contentType = body.contentType;
        task->code = 200;
    }
    impl->func(task);
}

} // namespace

FetcherOptions::FetcherOptions()
    : threads(1),
      timeout(-1),
      maxCacheConections(0),
      maxHostConnections(0),
      maxTotalConections(3),
      pipelining(2)
{}

Fetcher::~Fetcher()
{}

Fetcher *Fetcher::create(const FetcherOptions &options)
{
    return new FetcherImpl(options);
}

} // namespace vts
