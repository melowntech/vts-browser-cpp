#include <queue>

#include "height.h"
#include "map.h"

namespace melown
{

HeightRequest::HeightRequest() : result(0)
{}

namespace
{

class HeightRequestQueueImpl : public HeightRequestQueue
{
public:
    class HeightTask : public HeightRequest
    {
    public:
        // todo
    };
    
    HeightRequestQueueImpl(MapImpl *map, FindNavTileUrl findNavTileUrl)
        : map(map), findNavTileUrl(findNavTileUrl)
    {}
    
    void push(const vec2 &navPos) override
    {
        // todo
    }
    
    bool pop(HeightRequest &result) override
    {
        if (tasks.empty())
            return false;
        HeightTask &task = *tasks.front();
        
        // todo
    }
    
    FindNavTileUrl findNavTileUrl;
    std::queue<std::shared_ptr<HeightTask>> tasks;
    MapImpl *map;
};

}

HeightRequestQueue *HeightRequestQueue::create(MapImpl *map,
                                               FindNavTileUrl findNavTileUrl)
{
    return new HeightRequestQueueImpl(map, findNavTileUrl);
}

} // namespace melown
