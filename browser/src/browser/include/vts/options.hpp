#ifndef OPTIONS_H_kwegfdzvgsdfj
#define OPTIONS_H_kwegfdzvgsdfj

#include "foundation.hpp"

namespace vts
{

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
    uint32 maxResourcesMemory;
    uint32 maxConcurrentDownloads;
    uint32 maxNodeUpdatesPerFrame;
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
