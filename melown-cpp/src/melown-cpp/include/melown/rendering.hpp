#ifndef RENDERING_H_erbgfhugfsugf
#define RENDERING_H_erbgfhugfsugf

#include <vector>

#include "foundation.hpp"

namespace melown
{

class DrawTask
{
public:
    class GpuMesh *mesh;
    class GpuTexture *texColor;
    class GpuTexture *texMask;
    float mvp[16];
    float uvm[9];
    float color[3];
    float alpha;
    bool externalUv;
    
    DrawTask(class RenderTask *r, class MapImpl *m);
};

class DrawBatch
{
public:
    std::vector<DrawTask> opaque;
    std::vector<DrawTask> transparent;
    std::vector<DrawTask> wires;
};

} // namespace melown

#endif
