#ifndef MAP_H_jihsefk
#define MAP_H_jihsefk

#include <string>
#include <memory>
#include <functional>

#include "foundation.h"

namespace melown
{

class MELOWN_API MapFoundation
{
public:
    MapFoundation();
    virtual ~MapFoundation();
    
    std::function<std::shared_ptr<class GpuTexture>(const std::string &)>
            createTexture;
    std::function<std::shared_ptr<class GpuMesh>(const std::string &)>
            createMesh;
    
    void dataInitialize(class Fetcher *fetcher);
    bool dataTick();
    void dataFinalize();

    void renderInitialize();
    void renderTick(uint32 width, uint32 height);
    void renderFinalize();

    void setMapConfig(const std::string &mapConfigPath);
    void pan(const double value[3]);
    void rotate(const double value[3]);

    class MapStatistics &statistics();
    class MapOptions &options();
    class DrawBatch &drawBatch();

private:
    std::shared_ptr<class MapImpl> impl;
};

} // namespace melown

#endif
