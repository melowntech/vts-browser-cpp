#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>
#include <memory>

#include "foundation.h"

namespace melown
{

class MapImpl;

class MELOWN_API MapFoundation
{
public:
    MapFoundation(const std::string &mapConfigPath);
    virtual ~MapFoundation();

    void dataInitialize(class GpuContext *context, class Fetcher *fetcher);
    bool dataTick();
    void dataFinalize();

    void renderInitialize(class GpuContext *context);
    void renderTick(uint32 width, uint32 height);
    void renderFinalize();

    void pan(const double value[3]);
    void rotate(const double value[3]);

private:
    std::shared_ptr<MapImpl> impl;
};

} // namespace melown

#endif
