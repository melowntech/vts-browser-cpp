#ifndef FETCHERIMPL_H_djghfubhj
#define FETCHERIMPL_H_djghfubhj

#include <memory>
#include <renderer/fetcher.h>

namespace
{

class FetcherDetail;

} // namespace

class FetcherImpl : public melown::Fetcher
{
public:
    FetcherImpl();
    ~FetcherImpl();

    void initialize(Func func) override;
    void finalize() override;
    
    void fetch(melown::FetchTask *task) override;

    std::shared_ptr<FetcherDetail> impl;
};

#endif
