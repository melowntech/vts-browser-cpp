#ifndef FETCHERIMPL_H_djghfubhj
#define FETCHERIMPL_H_djghfubhj

#include <memory>
#include <vts-browser/fetcher.hpp>

class FetcherDetail;

class FetcherImpl : public vts::Fetcher
{
public:
    FetcherImpl();
    ~FetcherImpl();

    void initialize() override;
    void finalize() override;
    void fetch(const std::shared_ptr<vts::FetchTask> &task) override;

    std::shared_ptr<FetcherDetail> impl;
};

#endif
