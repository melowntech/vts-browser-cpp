#ifndef RENDERING_H_erbgfhugfsugf
#define RENDERING_H_erbgfhugfsugf

#include <vector>

#include "foundation.hpp"

namespace vts
{

class VTS_API DrawTask
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

class VTS_API DrawBatch
{
public:
    std::vector<DrawTask> opaque;
    std::vector<DrawTask> transparent;
    std::vector<DrawTask> wires;
};

} // namespace vts

#endif
