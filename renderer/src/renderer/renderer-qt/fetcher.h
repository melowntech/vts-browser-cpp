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

    void initialize(Func func) override;
    void finalize() override;
    
    void setOptions(const FetcherOptions &options);
    void fetch(melown::FetchTask *task) override;

    FetcherDetail *impl;
};

#endif
