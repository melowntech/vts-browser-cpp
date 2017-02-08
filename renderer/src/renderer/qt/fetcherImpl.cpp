#include "fetcherImpl.h"

namespace
{
    class FetcherDetail
    {
    public:
        FetcherDetail()
        {

        }

        ~FetcherDetail()
        {

        }

        void fetch(const std::string &name, FetcherImpl::Func func)
        {

        }
    };
}

FetcherImpl::FetcherImpl() : impl(nullptr)
{
    impl = new FetcherDetail();
}

FetcherImpl::~FetcherImpl()
{
    delete impl;
}

void FetcherImpl::fetch(const std::string &name, Func func)
{
    return impl->fetch(name, func);
}
