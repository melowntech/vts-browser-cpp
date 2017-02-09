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
    void setCallback(Func func) override;
    void fetch(const std::string &name) override;

    FetcherDetail *impl;
};

#endif
