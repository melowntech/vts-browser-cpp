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
    [StructLayout(LayoutKind.Sequential)]
    public struct CameraBase
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)] public double[] view;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)] public double[] proj;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public double[] eye;
        public double near, far;
        public double aspect;
        public double fov;
        [MarshalAs(UnmanagedType.I1)] public bool mapProjected;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DrawBase
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)] public float[] mv;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)] public float[] uvm;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)] public float[] color;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)] public float[] uvClip;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public float[] center;
        [MarshalAs(UnmanagedType.I1)] public bool externalUv;
        [MarshalAs(UnmanagedType.I1)] public bool flatShading;
    }

    public struct DrawTask
    {
        public DrawBase data;
        public Object mesh;
        public Object texColor;
        public Object texMask;
    }

    public class Draws
    {
        public CameraBase camera;
        public List<DrawTask> opaque;
        public List<DrawTask> transparent;
        public List<DrawTask> geodata;
        public List<DrawTask> infographics;

        private Object Load(IntPtr ptr)
        {
            Util.CheckError();
            if (ptr == IntPtr.Zero)
                return null;
            GCHandle hnd = GCHandle.FromIntPtr(ptr);
            return hnd.Target;
        }

        private void Load(out List<DrawTask> tasks, IntPtr group)
        {
            Util.CheckError();
            uint cnt = BrowserInterop.vtsDrawsCount(group);
            Util.CheckError();
            tasks = new List<DrawTask>((int)cnt);
            for (uint i = 0; i < cnt; i++)
            {
                DrawTask t;
                t.mesh = Load(BrowserInterop.vtsDrawsMesh(group, i));
                t.texColor = Load(BrowserInterop.vtsDrawsTexColor(group, i));
                t.texMask = Load(BrowserInterop.vtsDrawsTexMask(group, i));
                IntPtr dataPtr = BrowserInterop.vtsDrawsDetail(group, i);
                Util.CheckError();
                t.data = (DrawBase)Marshal.PtrToStructure(dataPtr, typeof(DrawBase));
                tasks.Add(t);
            }
            BrowserInterop.vtsDrawsDestroy(group);
            Util.CheckError();
        }

        public Draws(Map map)
        {
            IntPtr camPtr = BrowserInterop.vtsDrawsCamera(map.Handle);
            Util.CheckError();
            camera = (CameraBase)Marshal.PtrToStructure(camPtr, typeof(CameraBase));
            Load(out opaque, BrowserInterop.vtsDrawsOpaque(map.Handle));
            Load(out transparent, BrowserInterop.vtsDrawsTransparent(map.Handle));
            Load(out geodata, BrowserInterop.vtsDrawsGeodata(map.Handle));
            Load(out infographics, BrowserInterop.vtsDrawsInfographics(map.Handle));
        }
    }
}
