#ifndef DRAWS_H_qwedfzugvsdfh
#define DRAWS_H_qwedfzugvsdfh

#include <vector>
#include <memory>

#include "foundation.hpp"

namespace vts
{

class VTS_API DrawTask
{
public:
    std::shared_ptr<void> mesh;
    std::shared_ptr<void> texColor;
    std::shared_ptr<void> texMask;
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
