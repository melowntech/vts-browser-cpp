#ifndef FETCHERIMPL_H_djghfubhj
#define FETCHERIMPL_H_djghfubhj

#include <renderer/fetcher.h>

namespace
{
    class FetcherDetail;
}

class FetcherImpl : public melown::Fetcher
{
public:
    FetcherImpl();
    ~FetcherImpl();
    void fetch(const std::string &name, Func func) override;

    FetcherDetail *impl;
};

#endif
