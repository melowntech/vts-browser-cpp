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
            handle = BrowserInterop.vtsMapCreate(createOptions);
            Util.CheckError();
            AssignInternalDelegates();
            AssignInternalCallbacks();
        }

        public void SetMapConfigPath(string mapConfigPath, string authPath = "", string sriPath = "")
        {
            BrowserInterop.vtsMapSetConfigPaths(Handle, mapConfigPath, authPath, sriPath);
            Util.CheckError();
        }

        public string GetMapConfigPath()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetConfigPath(Handle));
            Util.CheckError();
            return res;
        }

        public bool GetMapConfigAvailable()
        {
            bool res = BrowserInterop.vtsMapGetConfigAvailable(Handle);
            Util.CheckError();
            return res;
        }

        public bool GetMapConfigReady()
        {
            bool res = BrowserInterop.vtsMapGetConfigReady(Handle);
            Util.CheckError();
            return res;
        }

        public bool GetMapRenderComplete()
        {
            bool res = BrowserInterop.vtsMapGetRenderComplete(Handle);
            Util.CheckError();
            return res;
        }

        public double GetMapRenderProgress()
        {
            double res = BrowserInterop.vtsMapGetRenderProgress(Handle);
            Util.CheckError();
            return res;
        }
        
        public void DataInitialize()
        {
            BrowserInterop.vtsMapDataInitialize(Handle, IntPtr.Zero);
            Util.CheckError();
        }

        public void DataTick()
        {
            BrowserInterop.vtsMapDataTick(Handle);
            Util.CheckError();
        }

        public void DataDeinitialize()
        {
            BrowserInterop.vtsMapDataFinalize(Handle);
            Util.CheckError();
        }

        public void RenderInitialize()
        {
            BrowserInterop.vtsMapRenderInitialize(Handle);
            Util.CheckError();
        }

        public void RenderTickPrepare(double elapsedTime)
        {
            BrowserInterop.vtsMapRenderTickPrepare(Handle, elapsedTime);
            Util.CheckError();
        }

        public void RenderTickRender()
        {
            BrowserInterop.vtsMapRenderTickRender(Handle);
            Util.CheckError();
        }

        public void RenderDeinitialize()
        {
            BrowserInterop.vtsMapRenderFinalize(Handle);
            Util.CheckError();
        }

        public void SetWindowSize(uint width, uint height)
        {
            BrowserInterop.vtsMapSetWindowSize(Handle, width, height);
            Util.CheckError();
        }

        public string GetOptions()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetOptions(Handle));
            Util.CheckError();
            return res;
        }

        public string GetStatistics()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetStatistics(Handle));
            Util.CheckError();
            return res;
        }

        public string GetCredits()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetCredits(Handle));
            Util.CheckError();
            return res;
        }

        public string GetCreditsShort()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetCreditsShort(Handle));
            Util.CheckError();
            return res;
        }

        public string GetCreditsFull()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetCreditsFull(Handle));
            Util.CheckError();
            return res;
        }

        public void SetOptions(string json)
        {
            BrowserInterop.vtsMapSetOptions(Handle, json);
            Util.CheckError();
        }

        public void Pan(double[] value)
        {
            Util.CheckArray(value, 3);
            BrowserInterop.vtsMapPan(Handle, value);
            Util.CheckError();
        }

        public void Rotate(double[] value)
        {
            Util.CheckArray(value, 3);
            BrowserInterop.vtsMapRotate(Handle, value);
            Util.CheckError();
        }

        public void Zoom(double value)
        {
            BrowserInterop.vtsMapZoom(Handle, value);
            Util.CheckError();
        }
        
        public void ResetPositionAltitude()
        {
            BrowserInterop.vtsMapResetPositionAltitude(Handle);
            Util.CheckError();
        }

        public void ResetNavigationMode()
        {
            BrowserInterop.vtsMapResetNavigationMode(Handle);
            Util.CheckError();
        }

        public void SetPositionSubjective(bool subjective, bool convert)
        {
            BrowserInterop.vtsMapSetPositionSubjective(Handle, subjective, convert);
            Util.CheckError();
        }

        public void SetPositionPoint(double[] point)
        {
            Util.CheckArray(point, 3);
            BrowserInterop.vtsMapSetPositionPoint(Handle, point);
            Util.CheckError();
        }

        public void SetPositionRotation(double[] rot)
        {
            Util.CheckArray(rot, 3);
            BrowserInterop.vtsMapSetPositionRotation(Handle, rot);
            Util.CheckError();
        }

        public void SetPositionViewExtent(double viewExtent)
        {
            BrowserInterop.vtsMapSetPositionViewExtent(Handle, viewExtent);
            Util.CheckError();
        }

        public void SetPositionFov(double fov)
        {
            BrowserInterop.vtsMapSetPositionFov(Handle, fov);
            Util.CheckError();
        }

        public void SetPositionJson(string position)
        {
            BrowserInterop.vtsMapSetPositionJson(Handle, position);
            Util.CheckError();
        }

        public void SetPositionUrl(string position)
        {
            BrowserInterop.vtsMapSetPositionUrl(Handle, position);
            Util.CheckError();
        }

        public bool GetPositionSubjective()
        {
            bool res = BrowserInterop.vtsMapGetPositionSubjective(Handle);
            Util.CheckError();
            return res;
        }

        public double[] GetPositionPoint()
        {
            double[] res = new double[3];
            BrowserInterop.vtsMapGetPositionPoint(Handle, res);
            Util.CheckError();
            return res;
        }

        public double[] GetPositionRotation()
        {
            double[] res = new double[3];
            BrowserInterop.vtsMapGetPositionRotation(Handle, res);
            Util.CheckError();
            return res;
        }

        public double[] GetPositionRotationLimited()
        {
            double[] res = new double[3];
            BrowserInterop.vtsMapGetPositionRotationLimited(Handle, res);
            Util.CheckError();
            return res;
        }

        public double GetPositionViewExtent()
        {
            double res = BrowserInterop.vtsMapGetPositionViewExtent(Handle);
            Util.CheckError();
            return res;
        }

        public double GetPositionFov()
        {
            double res = BrowserInterop.vtsMapGetPositionFov(Handle);
            Util.CheckError();
            return res;
        }

        public string GetPositionJson()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetPositionJson(Handle));
            Util.CheckError();
            return res;
        }

        public string GetPositionUrl()
        {
            string res = Util.CStringToSharp(BrowserInterop.vtsMapGetPositionUrl(Handle));
            Util.CheckError();
            return res;
        }

        public double[] Convert(double[] point, Srs srsFrom, Srs srsTo)
        {
            Util.CheckArray(point, 3);
            double[] res = new double[3];
            BrowserInterop.vtsMapConvert(Handle, point, res, (uint)srsFrom, (uint)srsTo);
            Util.CheckError();
            return res;
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

        protected virtual void Dispose(bool disposing)
        {
            if (!disposed)
            {
                if (Handle != null)
                {
                    BrowserInterop.vtsMapDestroy(Handle);
                    Util.CheckError();
                    handle = IntPtr.Zero;
                }
                disposed = true;
            }
        }

        private bool disposed = false;
        protected IntPtr handle;
        public IntPtr Handle { get { return handle; } }
    }
}
