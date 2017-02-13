#ifndef GLCLASSES_H_jhsegf
#define GLCLASSES_H_jhsegf

#include <string>

#include <math/math_all.hpp>

#include "foundation.h"

namespace melown
{
    class GpuResource
    {
    public:
        GpuResource();
        virtual ~GpuResource();

        virtual void loadToGpu(const std::string &name, class Map *base);

        uint32 memoryCost;
        bool ready;
    };

    class GpuShader : public GpuResource
    {
    public:
        virtual void bind() = 0;
        virtual void loadShaders(const std::string &vertexShader, const std::string &fragmentShader) = 0;

        void loadToGpu(const std::string &name, class Map *base) override;
    };

    class GpuTexture : public GpuResource
    {
    public:
        virtual void bind() = 0;
        virtual void loadTexture(void *buffer, uint32 size) = 0;

        void loadToGpu(const std::string &name, class Map *base) override;
    };

    class GpuMeshSpec
    {
    public:
        GpuMeshSpec();
        const uint16 *indexBuffer;
        void *vertexBuffer;
        uint32 vertexCount;
        uint32 vertexSize;
        uint32 indexCount;
        enum class FaceMode
        {
            Points = 0x0000,
            Lines = 0x0001,
            LineStrip = 0x0003,
            Triangles = 0x0004,
            TriangleStrip = 0x0005,
            TriangleFan = 0x0006,
        } faceMode;
        struct VertexAttribute
        {
            VertexAttribute();
            uint32 offset; // in bytes
            uint32 stride; // in bytes
            uint32 components; // 1, 2, 3 or 4
            enum class Type
            {
                Float = 0x1406,
            } type;
            bool enable;
            bool normalized;
        } attributes[2];
    };

    class GpuSubMesh : public GpuResource
    {
    public:
        virtual void draw() = 0;
        virtual void loadSubMesh(const GpuMeshSpec &spec) = 0;
    };

    class GpuMeshAggregate : public GpuResource
    {
    public:
        std::vector<GpuSubMesh*> submeshes;

        virtual void loadMeshAggregate() = 0;

        void loadToGpu(const std::string &name, class Map *base) override;
    };
}

#endif
