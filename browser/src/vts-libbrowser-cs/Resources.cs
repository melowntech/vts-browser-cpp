/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace vts
{
    public enum GpuType
    {
        // compatible with OpenGL
        Byte = 0x1400,
        UnsignedByte = 0x1401,
        Short = 0x1402, // two bytes
        UnsignedShort = 0x1403,
        Int = 0x1404, // four bytes
        UnsignedInt = 0x1405,
        Float = 0x1406, // four bytes
    };

    public enum FilterMode
    {
        // compatible with OpenGL
        Nearest = 0x2600,
        Linear = 0x2601,
        NearestMipmapNearest = 0x2700,
        LinearMipmapNearest = 0x2701,
        NearestMipmapLinear = 0x2702,
        LinearMipmapLinear = 0x2703,
    };

    public enum WrapMode
    {
        // compatible with OpenGL
        Repeat = 0x2901,
        ClampToEdge = 0x812F,
        ClampToBorder = 0x812D,
        MirroredRepeat = 0x8370,
        MirrorClampToEdge = 0x8743,
    };

    public enum FaceMode
    {
        // compatible with OpenGL
        Points = 0x0000,
        Lines = 0x0001,
        LineStrip = 0x0003,
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        TriangleFan = 0x0006,
    };

    public class Texture
    {
        public uint width;
        public uint height;
        public uint components;
        public GpuType type;
        public FilterMode filterMode;
        public WrapMode wrapMode;
        public byte[] data;

        public void Load(IntPtr handle)
        {
            BrowserInterop.vtsTextureGetResolution(handle, ref width, ref height, ref components);
            Util.CheckInterop();
            type = (GpuType)BrowserInterop.vtsTextureGetType(handle);
            Util.CheckInterop();
            filterMode = (FilterMode)BrowserInterop.vtsTextureGetFilterMode(handle);
            Util.CheckInterop();
            wrapMode = (WrapMode)BrowserInterop.vtsTextureGetWrapMode(handle);
            Util.CheckInterop();
            IntPtr bufPtr = IntPtr.Zero;
            uint bufSize = 0;
            BrowserInterop.vtsTextureGetBuffer(handle, ref bufPtr, ref bufSize);
            Util.CheckInterop();
            data = new byte[bufSize];
            Marshal.Copy(bufPtr, data, 0, (int)bufSize);
        }
    }

    public struct VertexAttribute
    {
        public uint offset; // in bytes
        public uint stride; // in bytes
        public uint components; // 1, 2, 3 or 4
        public GpuType type;
        public bool enable;
        public bool normalized;
    };

    public class Mesh
    {
        public FaceMode faceMode;
        public List<VertexAttribute> attributes;
        public uint verticesCount;
        public uint indicesCount;
        public byte[] vertices;
        public ushort[] indices;

        public void Load(IntPtr handle)
        {
            faceMode = (FaceMode)BrowserInterop.vtsMeshGetFaceMode(handle);
            Util.CheckInterop();
            IntPtr bufPtr = IntPtr.Zero;
            uint bufSize = 0;
            BrowserInterop.vtsMeshGetIndices(handle, ref bufPtr, ref bufSize, ref indicesCount);
            Util.CheckInterop();
            if (indicesCount > 0)
            {
                short[] tmp = new short[indicesCount];
                Marshal.Copy(bufPtr, tmp, 0, (int)indicesCount);
                indices = new ushort[indicesCount];
                Buffer.BlockCopy(tmp, 0, indices, 0, (int)indicesCount * 2);
            }
            BrowserInterop.vtsMeshGetVertices(handle, ref bufPtr, ref bufSize, ref verticesCount);
            Util.CheckInterop();
            vertices = new byte[bufSize];
            Marshal.Copy(bufPtr, vertices, 0, (int)bufSize);
            attributes = new List<VertexAttribute>(4);
            for (uint i = 0; i < 4; i++)
            {
                VertexAttribute a;
                a.offset = 0;
                a.stride = 0;
                a.components = 0;
                a.enable = false;
                a.normalized = false;
                uint type = 0;
                BrowserInterop.vtsMeshGetAttribute(handle, i, ref a.offset, ref a.stride, ref a.components, ref type, ref a.enable, ref a.normalized);
                Util.CheckInterop();
                a.type = (GpuType)type;
                attributes.Add(a);
            }
        }
    }
}
