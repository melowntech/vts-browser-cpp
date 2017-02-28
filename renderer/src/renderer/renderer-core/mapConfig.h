#ifndef MAPCONFIG_H_qwedfzcbk
#define MAPCONFIG_H_qwedfzcbk

#include <string>

#include <vts-libs/vts/mapconfig.hpp>

#include <renderer/foundation.h>
#include <renderer/resource.h>

namespace melown
{
    class MapConfig : public Resource, public vtslibs::vts::MapConfig
    {
    public:
        MapConfig(const std::string &name);

        void load(class MapImpl *base) override;
    };
}

#endif
