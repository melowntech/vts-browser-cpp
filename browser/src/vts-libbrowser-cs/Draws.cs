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

    public struct Atmosphere
    {
        public double thickness;
        public double horizontalExponent;
        public double verticalExponent;
        public float[] colorLow;
        public float[] colorHigh;

        public void Load(Map map)
        {
            if (colorLow == null)
            {
                colorLow = new float[4];
                colorHigh = new float[4];
            }
            BrowserInterop.vtsCelestialAtmosphere(map.Handle, out thickness, out horizontalExponent, out verticalExponent, colorLow, colorHigh);
            Util.CheckInterop();
        }
    }

    public struct Celestial
    {
        public Atmosphere atmosphere;
        public string name;
        public double majorRadius;
        public double minorRadius;

        public void Load(Map map)
        {
            atmosphere.Load(map);
            name = Util.CheckString(BrowserInterop.vtsCelestialName(map.Handle));
            majorRadius = BrowserInterop.vtsCelestialMajorRadius(map.Handle);
            Util.CheckInterop();
            minorRadius = BrowserInterop.vtsCelestialMinorRadius(map.Handle);
            Util.CheckInterop();
        }
    }

    public class Draws
    {
        public CameraBase camera;
        public Celestial celestial;
        public List<DrawTask> opaque;
        public List<DrawTask> transparent;
        public List<DrawTask> geodata;
        public List<DrawTask> infographics;

        private Object Load(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
                return null;
            GCHandle hnd = GCHandle.FromIntPtr(ptr);
            return hnd.Target;
        }

        private void Load(ref List<DrawTask> tasks, IntPtr group)
        {
            Util.CheckInterop();
            try
            {
                uint cnt = BrowserInterop.vtsDrawsCount(group);
                Util.CheckInterop();
                if (tasks == null)
                    tasks = new List<DrawTask>((int)cnt);
                else
                    tasks.Clear();
                for (uint i = 0; i < cnt; i++)
                {
                    DrawTask t;
                    IntPtr pm, ptc, ptm;
                    IntPtr dataPtr = BrowserInterop.vtsDrawsAllInOne(group, i, out pm, out ptc, out ptm);
                    Util.CheckInterop();
                    t.data = (DrawBase)Marshal.PtrToStructure(dataPtr, typeof(DrawBase));
                    t.mesh = Load(pm);
                    t.texColor = Load(ptc);
                    t.texMask = Load(ptm);
                    tasks.Add(t);
                }
            }
            finally
            {
                BrowserInterop.vtsDrawsDestroy(group);
                Util.CheckInterop();
            }
        }

        public void Load(Map map)
        {
            IntPtr camPtr = BrowserInterop.vtsDrawsCamera(map.Handle);
            Util.CheckInterop();
            camera = (CameraBase)Marshal.PtrToStructure(camPtr, typeof(CameraBase));
            celestial.Load(map);
            Load(ref opaque, BrowserInterop.vtsDrawsOpaque(map.Handle));
            Load(ref transparent, BrowserInterop.vtsDrawsTransparent(map.Handle));
            Load(ref geodata, BrowserInterop.vtsDrawsGeodata(map.Handle));
            Load(ref infographics, BrowserInterop.vtsDrawsInfographics(map.Handle));
        }
    }
}
