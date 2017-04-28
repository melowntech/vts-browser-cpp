#ifndef OPTIONS_H_kwegfdzvgsdfj
#define OPTIONS_H_kwegfdzvgsdfj

#include "foundation.hpp"

namespace vts
{

VTS_API void setLogMask(const std::string &mask);

class VTS_API MapOptions
{
public:
    MapOptions();
    ~MapOptions();
    
    double maxTexelToPixelScale;
    double positionViewExtentMin;
    double positionViewExtentMax;
    double cameraSensitivityPan;
    double cameraSensitivityAltitude;
    double cameraSensitivityZoom;
    double cameraSensitivityRotate;
    double cameraInertiaPan;
    double cameraInertiaAltitude;
    double cameraInertiaZoom;
    double cameraInertiaRotate;
    uint64 maxResourcesMemory;
    uint32 maxConcurrentDownloads;
    uint32 maxNodeUpdatesPerTick;
    uint32 maxResourceProcessesPerTick;
    uint32 navigationSamplesPerViewExtent;
    
    bool renderSurrogates;
    bool renderMeshBoxes;
    bool renderTileBoxes;
    bool renderObjectPosition;
    
    bool debugDetachedCamera;
    bool debugDisableMeta5;
};

} // namespace vts

#endif
