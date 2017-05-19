#ifndef OPTIONS_H_kwegfdzvgsdfj
#define OPTIONS_H_kwegfdzvgsdfj

#include <string>
#include <functional>

#include "foundation.hpp"

namespace vts
{

VTS_API void setLogMask(const std::string &mask);

class VTS_API MapCreateOptions
{
public:
    MapCreateOptions(const std::string &clientId);
    
    std::string clientId;
    std::string cachePath;
    bool disableCache;
};

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

class VTS_API MapCallbacks
{
public:
    // function callback to upload a texture to gpu
    std::function<void(class ResourceInfo &, const class GpuTextureSpec &)>
            loadTexture;
    /// function callback to upload a mesh to gpu
    std::function<void(class ResourceInfo &, const class GpuMeshSpec &)>
            loadMesh;
    
    /// function callbacks for camera overrides (all in physical srs)
    std::function<void(double*)> cameraOverrideEye;
    std::function<void(double*)> cameraOverrideTarget;
    std::function<void(double*)> cameraOverrideUp;
    std::function<void(double&, double&, double&, double&)>
                                        cameraOverrideFovAspectNearFar;
    std::function<void(double*)> cameraOverrideView;
    std::function<void(double*)> cameraOverrideProj;
};

} // namespace vts

#endif
