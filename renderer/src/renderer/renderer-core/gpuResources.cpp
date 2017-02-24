#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

#include <renderer/gpuResources.h>

#include "map.h"
#include "cache.h"
#include "resourceManager.h"
#include "buffer.h"

#include <jpeglib.h>

namespace melown
{
    GpuShader::GpuShader(const std::string &name) : Resource(name)
    {}

    void GpuShader::load(MapImpl *base)
    {
        void *bv = nullptr, *bf = nullptr;
        uint32 sv = 0, sf = 0;
        Cache::Result rv = base->cache->read(name + ".vert.glsl", bv, sv);
        Cache::Result rf = base->cache->read(name + ".frag.glsl", bf, sf);
        if (rv == Cache::Result::error || rf == Cache::Result::error)
        {
            state = State::errorDownload;
            return;
        }
        if (rv == Cache::Result::ready && rf == Cache::Result::ready)
        {
            std::string vert((char*)bv, sv);
            std::string frag((char*)bf, sf);
            loadShaders(vert, frag);
        }
    }

    GpuTextureSpec::GpuTextureSpec() : width(0), height(0), components(0), bufferSize(0), buffer(nullptr)
    {}

    GpuTexture::GpuTexture(const std::string &name) : Resource(name)
    {}

    void GpuTexture::load(MapImpl *base)
    {
        void *buffer = nullptr;
        uint32 size = 0;
        switch (base->cache->read(name, buffer, size))
        {
        case Cache::Result::ready:
        {
            GpuTextureSpec spec;
            jpeg_decompress_struct info;
            jpeg_error_mgr errmgr;
            info.err = jpeg_std_error(&errmgr);
            jpeg_create_decompress(&info);
            jpeg_mem_src(&info, (unsigned char*)buffer, size);
            jpeg_read_header(&info, TRUE);
            jpeg_start_decompress(&info);
            spec.width = info.output_width;
            spec.height = info.output_height;
            spec.components = info.num_components;
            uint32 lineSize = spec.components * spec.width;
            Buffer buf(lineSize * spec.height);
            while (info.output_scanline < info.output_height)
            {
                unsigned char *ptr[1];
                ptr[0] = (unsigned char*)buf.data + lineSize * (spec.height - info.output_scanline - 1);
                jpeg_read_scanlines(&info, ptr, 1);
            }
            jpeg_finish_decompress(&info);
            jpeg_destroy_decompress(&info);
            spec.buffer = buf.data;
            spec.bufferSize = buf.size;
            loadTexture(spec);
        } return;
        case Cache::Result::error:
            state = State::errorDownload;
            return;
        }
    }

    GpuMeshSpec::GpuMeshSpec() : indexBufferData(nullptr), vertexBufferData(nullptr), verticesCount(0), vertexBufferSize(0), indicesCount(0), faceMode(FaceMode::Triangles)
    {}

    GpuMeshSpec::VertexAttribute::VertexAttribute() : offset(0), stride(0), components(0), type(Type::Float), enable(false), normalized(false)
    {}

    GpuMeshRenderable::GpuMeshRenderable(const std::string &name) : Resource(name)
    {}

    void GpuMeshRenderable::load(MapImpl *base)
    {}
}
