#ifndef FETCHER_H_wjehfduj
#define FETCHER_H_wjehfduj

#include <string>
#include <functional>

#include "foundation.h"

namespace melown
{
    enum class FetchType;

    class MELOWN_API Fetcher
    {
    public:
        typedef std::function<void(FetchType, const std::string &, const char *, uint32)> Func;

        virtual ~Fetcher() {}
        virtual void setCallback(Func func) = 0;
        virtual void fetch(FetchType type, const std::string &name) = 0;
    };
}

#endif
