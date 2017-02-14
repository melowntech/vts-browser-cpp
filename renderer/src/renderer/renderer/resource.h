#ifndef RESOURCE_H_hjsghjefjkn
#define RESOURCE_H_hjsghjefjkn

#include <string>

#include "foundation.h"

namespace melown
{
    class Resource
    {
    public:
        Resource();
        virtual ~Resource();

        virtual void load(const std::string &name, class Map *base) = 0;

        uint32 ramMemoryCost;
        uint32 gpuMemoryCost;
        bool ready;
    };
}

#endif
