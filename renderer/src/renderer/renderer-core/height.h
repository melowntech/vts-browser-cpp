#ifndef HEIGHT_H_qwgfrbsa
#define HEIGHT_H_qwgfrbsa

#include <functional>
#include <renderer/foundation.h>

#include "math.h"

namespace melown
{

class HeightRequest
{
public:
    HeightRequest();
    HeightRequest(const HeightRequest &other) = default;
    
    vec2 navPos;
    double result;
};

class HeightRequestQueue
{
public:
    typedef std::function<bool()> FindNavTileUrl;
    
    static HeightRequestQueue *create(class MapImpl *map, FindNavTileUrl findNavTileUrl);
    
    virtual void push(const vec2 &navPos) = 0;
    virtual bool pop(HeightRequest &result) = 0;
};

} // namespace melown

#endif
