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
using System.Diagnostics;
using System.Collections.Generic;

namespace vts
{
    public enum Srs : uint
    {
        Physical,
        Navigation,
        Public,
        Search,
        Custom1,
        Custom2,
    }

    public partial class Map : IDisposable
    {
        public Map(string createOptions)
        {
            handle = BrowserInterop.vtsMapCreate(createOptions, IntPtr.Zero);
            Util.CheckInterop();
#if ENABLE_IL2CPP
            GlobalMaps.Add(handle, this);
#endif
            AssignInternalDelegates();
            AssignInternalCallbacks();
        }

#if ENABLE_IL2CPP
        private static Dictionary<IntPtr, Map> GlobalMaps = new Dictionary<IntPtr, Map>();
        private static Map GetMap(IntPtr h)
        {
            Debug.Assert(GlobalMaps[h] != null);
            return GlobalMaps[h];
        }
#else
        private Map GetMap(IntPtr h)
        {
            Debug.Assert(Handle == h);
            return this;
        }
#endif

        public void SetMapConfigPath(string mapConfigPath, string authPath = "", string sriPath = "")
        {
            BrowserInterop.vtsMapSetConfigPaths(Handle, mapConfigPath, authPath, sriPath);
            Util.CheckInterop();
        }

        public string GetMapConfigPath()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetConfigPath(Handle));
        }

        public bool GetMapConfigAvailable()
        {
            bool res = BrowserInterop.vtsMapGetConfigAvailable(Handle);
            Util.CheckInterop();
            return res;
        }

        public bool GetMapConfigReady()
        {
            bool res = BrowserInterop.vtsMapGetConfigReady(Handle);
            Util.CheckInterop();
            return res;
        }

        public bool GetMapRenderComplete()
        {
            bool res = BrowserInterop.vtsMapGetRenderComplete(Handle);
            Util.CheckInterop();
            return res;
        }

        public double GetMapRenderProgress()
        {
            double res = BrowserInterop.vtsMapGetRenderProgress(Handle);
            Util.CheckInterop();
            return res;
        }
        
        public void DataInitialize()
        {
            BrowserInterop.vtsMapDataInitialize(Handle);
            Util.CheckInterop();
        }

        public void DataTick()
        {
            BrowserInterop.vtsMapDataTick(Handle);
            Util.CheckInterop();
        }

        public void DataDeinitialize()
        {
            BrowserInterop.vtsMapDataFinalize(Handle);
            Util.CheckInterop();
        }

        public void DataAllRun()
        {
            BrowserInterop.vtsMapDataAllRun(Handle);
            Util.CheckInterop();
        }

        public void RenderInitialize()
        {
            BrowserInterop.vtsMapRenderInitialize(Handle);
            Util.CheckInterop();
        }

        public void RenderTickPrepare(double elapsedTime)
        {
            BrowserInterop.vtsMapRenderTickPrepare(Handle, elapsedTime);
            Util.CheckInterop();
        }

        public void RenderTickRender()
        {
            BrowserInterop.vtsMapRenderTickRender(Handle);
            Util.CheckInterop();
        }

        public void RenderTickColliders()
        {
            BrowserInterop.vtsMapRenderTickColliders(Handle);
            Util.CheckInterop();
        }

        public void RenderDeinitialize()
        {
            BrowserInterop.vtsMapRenderFinalize(Handle);
            Util.CheckInterop();
        }

        public void SetWindowSize(uint width, uint height)
        {
            BrowserInterop.vtsMapSetWindowSize(Handle, width, height);
            Util.CheckInterop();
        }

        public string GetOptions()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetOptions(Handle));
        }

        public string GetStatistics()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetStatistics(Handle));
        }

        public string GetCredits()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetCredits(Handle));
        }

        public string GetCreditsShort()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetCreditsShort(Handle));
        }

        public string GetCreditsFull()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetCreditsFull(Handle));
        }

        public void SetOptions(string json)
        {
            BrowserInterop.vtsMapSetOptions(Handle, json);
            Util.CheckInterop();
        }

        public void Pan(double[] value)
        {
            Util.CheckArray(value, 3);
            BrowserInterop.vtsMapPan(Handle, value);
            Util.CheckInterop();
        }

        public void Rotate(double[] value)
        {
            Util.CheckArray(value, 3);
            BrowserInterop.vtsMapRotate(Handle, value);
            Util.CheckInterop();
        }

        public void Zoom(double value)
        {
            BrowserInterop.vtsMapZoom(Handle, value);
            Util.CheckInterop();
        }
        
        public void ResetPositionAltitude()
        {
            BrowserInterop.vtsMapResetPositionAltitude(Handle);
            Util.CheckInterop();
        }

        public void ResetNavigationMode()
        {
            BrowserInterop.vtsMapResetNavigationMode(Handle);
            Util.CheckInterop();
        }

        public void SetPositionSubjective(bool subjective, bool convert)
        {
            BrowserInterop.vtsMapSetPositionSubjective(Handle, subjective, convert);
            Util.CheckInterop();
        }

        public void SetPositionPoint(double[] point)
        {
            Util.CheckArray(point, 3);
            BrowserInterop.vtsMapSetPositionPoint(Handle, point);
            Util.CheckInterop();
        }

        public void SetPositionRotation(double[] rot)
        {
            Util.CheckArray(rot, 3);
            BrowserInterop.vtsMapSetPositionRotation(Handle, rot);
            Util.CheckInterop();
        }

        public void SetPositionViewExtent(double viewExtent)
        {
            BrowserInterop.vtsMapSetPositionViewExtent(Handle, viewExtent);
            Util.CheckInterop();
        }

        public void SetPositionFov(double fov)
        {
            BrowserInterop.vtsMapSetPositionFov(Handle, fov);
            Util.CheckInterop();
        }

        public void SetPositionJson(string position)
        {
            BrowserInterop.vtsMapSetPositionJson(Handle, position);
            Util.CheckInterop();
        }

        public void SetPositionUrl(string position)
        {
            BrowserInterop.vtsMapSetPositionUrl(Handle, position);
            Util.CheckInterop();
        }

        public bool GetPositionSubjective()
        {
            bool res = BrowserInterop.vtsMapGetPositionSubjective(Handle);
            Util.CheckInterop();
            return res;
        }

        public double[] GetPositionPoint()
        {
            double[] res = new double[3];
            BrowserInterop.vtsMapGetPositionPoint(Handle, res);
            Util.CheckInterop();
            return res;
        }

        public double[] GetPositionRotation()
        {
            double[] res = new double[3];
            BrowserInterop.vtsMapGetPositionRotation(Handle, res);
            Util.CheckInterop();
            return res;
        }

        public double[] GetPositionRotationLimited()
        {
            double[] res = new double[3];
            BrowserInterop.vtsMapGetPositionRotationLimited(Handle, res);
            Util.CheckInterop();
            return res;
        }

        public double GetPositionViewExtent()
        {
            double res = BrowserInterop.vtsMapGetPositionViewExtent(Handle);
            Util.CheckInterop();
            return res;
        }

        public double GetPositionFov()
        {
            double res = BrowserInterop.vtsMapGetPositionFov(Handle);
            Util.CheckInterop();
            return res;
        }

        public string GetPositionJson()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetPositionJson(Handle));
        }

        public string GetPositionUrl()
        {
            return Util.CheckString(BrowserInterop.vtsMapGetPositionUrl(Handle));
        }

        public double[] Convert(double[] point, Srs srsFrom, Srs srsTo)
        {
            Util.CheckArray(point, 3);
            double[] res = new double[3];
            BrowserInterop.vtsMapConvert(Handle, point, res, (uint)srsFrom, (uint)srsTo);
            Util.CheckInterop();
            return res;
        }
        
        public bool GetSearchable()
        {
            bool res = BrowserInterop.vtsMapGetSearchable(Handle);
            Util.CheckInterop();
            return res;
        }
        
        public SearchTask Search(string query)
        {
            IntPtr ptr = BrowserInterop.vtsMapSearch(Handle, query);
            Util.CheckInterop();
            return new SearchTask(ptr);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~Map()
        {
            Dispose(false);
        }

        protected void Dispose(bool disposing)
        {
            if (Handle != IntPtr.Zero)
            {
#if ENABLE_IL2CPP
                GlobalMaps.Remove(Handle);
#endif
                BrowserInterop.vtsMapDestroy(Handle);
                Util.CheckInterop();
                handle = IntPtr.Zero;
            }
        }

        protected IntPtr handle;
        public IntPtr Handle { get { return handle; } }
    }
}
