#ifndef FETCHERIMPL_H_djghfubhj
#define FETCHERIMPL_H_djghfubhj

#include <renderer/fetcher.h>

namespace
{
class FetcherDetail;
} // namespace

class FetcherOptions
{
public:
    std::string username;
    std::string password;
};

class FetcherImpl : public melown::Fetcher
{
public:
    FetcherImpl();
    ~FetcherImpl();

    void setOptions(const FetcherOptions &options);
    void setCallback(Func func) override;
    void fetch(const std::string &name) override;

    FetcherDetail *impl;
};

#endif
