#ifndef RENDERER_H_wekojgib
#define RENDERER_H_wekojgib

#include "math.h"

namespace melown
{

class Renderer
{
public:
    static Renderer *create(class MapImpl *map);

    Renderer();
    virtual ~Renderer();

    virtual void renderInitialize(class GpuContext *gpuContext) = 0;
    virtual void renderTick(uint32 width, uint32 height) = 0;
    virtual void renderFinalize() = 0;
    
    virtual void panAdjustZ(const vec2 &navPos) = 0;
};

} // namespace melown

#endif
