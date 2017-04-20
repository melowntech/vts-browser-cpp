#ifndef RESOURCES_H_jhsegfshg
#define RESOURCES_H_jhsegfshg

#include <memory>

#include "buffer.hpp"

namespace vts
{

class ResourceImpl;

class VTS_API Resource
{
public:
    Resource(const std::string &name);
    virtual ~Resource();

    virtual void load(class MapImpl *base) = 0;

    operator bool () const;
    
    const std::string name;
    uint32 ramMemoryCost;
    uint32 gpuMemoryCost;
    
    std::shared_ptr<ResourceImpl> impl;
};

class VTS_API GpuTextureSpec
{
public:
    GpuTextureSpec();
    GpuTextureSpec(const Buffer &buffer);

    uint32 width, height;
    uint32 components; // 1, 2, 3 or 4
    Buffer buffer;
    bool verticalFlip;
};

class VTS_API GpuTexture : public Resource
{
public:
    GpuTexture(const std::string &name);

    virtual void loadTexture(const GpuTextureSpec &spec) = 0;

    void load(class MapImpl *base) override;
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
    GpuMeshSpec(const Buffer &buffer);

    Buffer vertices;
    Buffer indices;
    VertexAttribute attributes[3];
    uint32 verticesCount;
    uint32 indicesCount;
    FaceMode faceMode;
};

class VTS_API GpuMesh : public Resource
{
public:
    GpuMesh(const std::string &name);

    virtual void loadMesh(const GpuMeshSpec &spec) = 0;

    void load(class MapImpl *base) override;
};

} // namespace vts

#endif
