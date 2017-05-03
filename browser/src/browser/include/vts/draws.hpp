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
    float color[4];
    bool externalUv;
    bool transparent;

    DrawTask();
    DrawTask(class RenderTask *r, class MapImpl *m);
};

class VTS_API MapDraws
{
public:
    MapDraws();
    
    std::vector<DrawTask> draws;
};

} // namespace vts

#endif
