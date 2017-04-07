#ifndef FETCHERIMPL_H_djghfubhj
#define FETCHERIMPL_H_djghfubhj

#include <memory>
#include <vts/fetcher.hpp>

namespace
{

class FetcherDetail;

} // namespace

class FetcherImpl : public vts::Fetcher
{
public:
    FetcherImpl();
    ~FetcherImpl();

    void initialize(Func func) override;
    void finalize() override;
    
    void fetch(vts::FetchTask *task) override;

    std::shared_ptr<FetcherDetail> impl;
};

#endif
