#ifndef RESOURCES_H_jhsegfshg
#define RESOURCES_H_jhsegfshg

#include <vector>
#include <memory>

#include "buffer.hpp"

namespace vts
{

class VTS_API ResourceInfo
{
public:
    ResourceInfo();
    
    std::shared_ptr<void> userData;
    uint32 ramMemoryCost;
    uint32 gpuMemoryCost;
};

class VTS_API GpuTextureSpec
{
public:
    GpuTextureSpec();
    GpuTextureSpec(const Buffer &buffer); // decode jpg or png file
    void verticalFlip();
    
    uint32 width, height;
    uint32 components; // 1, 2, 3 or 4
    Buffer buffer;
};

class VTS_API GpuMeshSpec
{
public:
    enum class FaceMode
    { // OpenGL constants
        Points = 0x0000,
        Lines = 0x0001,
        LineStrip = 0x0003,
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        TriangleFan = 0x0006,
    };

    struct VertexAttribute
    {
        enum class Type
        { // OpenGL constants
            Float = 0x1406,
            UnsignedByte = 0x1401,
        };

        VertexAttribute();
        uint32 offset; // in bytes
        uint32 stride; // in bytes
        uint32 components; // 1, 2, 3 or 4
        Type type;
        bool enable;
        bool normalized;
    };

    GpuMeshSpec();
    GpuMeshSpec(const Buffer &buffer); // decode obj file

    Buffer vertices;
    Buffer indices;
    std::vector<VertexAttribute> attributes;
    uint32 verticesCount;
    uint32 indicesCount;
    FaceMode faceMode;
};

} // namespace vts

#endif
