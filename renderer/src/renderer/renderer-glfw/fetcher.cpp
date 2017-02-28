#include <functional>
#include <unistd.h> // usleep

#include "fetcher.h"

#include <http/http.hpp>
#include <http/resourcefetcher.hpp>
#include <http/sink.hpp>

#include <boost/lockfree/queue.hpp>

namespace
{
    class FetcherImpl : public Fetcher
    {
    public:
        FetcherImpl() : fetcher(htt.fetcher())
        {
            htt.startClient(3);
            doneFunc = std::bind(&FetcherImpl::done, this, std::placeholders::_1);
        }

        ~FetcherImpl()
        {
            htt.stop();
        }

        void setCallback(melown::Fetcher::Func func) override
        {
            this->func = func;
        }

        void fetch(const std::string &name) override
        {
            fetcher.perform(http::ResourceFetcher::Query(name).timeout(-1), doneFunc);
        }

        void done(http::ResourceFetcher::MultiQuery &&queries)
        {
            for (auto &&q : queries)
            {
                /*
                if (!q.valid())
                {
                    LOG(info3) << q.ec();
                    if (q.exc())
                    {
                        try
                        {
                            std::rethrow_exception(q.exc());
                        }
                        catch(std::exception &e)
                        {
                            LOG(info3) << e.what();
                        }
                    }
                }
                */
                while (true)
                {
                    if (que.push(new http::ResourceFetcher::Query(std::move(q))))
                        break;
                    usleep(1000);
                }
            }
        }

        void tick() override
        {
            http::ResourceFetcher::Query *q = nullptr;
            if (que.pop(q))
            {
                if (q->valid())
                {
                    std::string b(q->get().data);
                    func(q->location(), b.c_str(), b.length());
                }
                else
                    func(q->location(), nullptr, 0);
                delete q;
            }
        }

        melown::Fetcher::Func func;
        http::Http htt;
        http::ResourceFetcher fetcher;
        http::ResourceFetcher::Done doneFunc;
        boost::lockfree::queue<http::ResourceFetcher::Query*, boost::lockfree::capacity<1024>> que;
    };
}

Fetcher *Fetcher::create()
{
    return new FetcherImpl();
}
