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
    public class Navigation : IDisposable
    {
        public Navigation(Camera camera)
        {
            handle = BrowserInterop.vtsNavigationCreate(camera.Handle);
            Util.CheckInterop();
        }

        public string GetOptions()
        {
            return Util.CheckString(BrowserInterop.vtsNavigationGetOptions(Handle));
        }

        public void SetOptions(string json)
        {
            BrowserInterop.vtsNavigationSetOptions(Handle, json);
            Util.CheckInterop();
        }

        public void Pan(double[] value)
        {
            Util.CheckArray(value, 3);
            BrowserInterop.vtsNavigationPan(Handle, value);
            Util.CheckInterop();
        }

        public void Rotate(double[] value)
        {
            Util.CheckArray(value, 3);
            BrowserInterop.vtsNavigationRotate(Handle, value);
            Util.CheckInterop();
        }

        public void Zoom(double value)
        {
            BrowserInterop.vtsNavigationZoom(Handle, value);
            Util.CheckInterop();
        }
        
        public void ResetAltitude()
        {
            BrowserInterop.vtsNavigationResetAltitude(Handle);
            Util.CheckInterop();
        }

        public void ResetNavigationMode()
        {
            BrowserInterop.vtsNavigationResetNavigationMode(Handle);
            Util.CheckInterop();
        }

        public void SetSubjective(bool subjective, bool convert)
        {
            BrowserInterop.vtsNavigationSetSubjective(Handle, subjective, convert);
            Util.CheckInterop();
        }

        public void SetPoint(double[] point)
        {
            Util.CheckArray(point, 3);
            BrowserInterop.vtsNavigationSetPoint(Handle, point);
            Util.CheckInterop();
        }

        public void SetRotation(double[] rot)
        {
            Util.CheckArray(rot, 3);
            BrowserInterop.vtsNavigationSetRotation(Handle, rot);
            Util.CheckInterop();
        }

        public void SetViewExtent(double viewExtent)
        {
            BrowserInterop.vtsNavigationSetViewExtent(Handle, viewExtent);
            Util.CheckInterop();
        }

        public void SetFov(double fov)
        {
            BrowserInterop.vtsNavigationSetFov(Handle, fov);
            Util.CheckInterop();
        }

        public void SetPositionJson(string position)
        {
            BrowserInterop.vtsNavigationSetPositionJson(Handle, position);
            Util.CheckInterop();
        }

        public void SetPositionUrl(string position)
        {
            BrowserInterop.vtsNavigationSetPositionUrl(Handle, position);
            Util.CheckInterop();
        }

        public bool GetSubjective()
        {
            bool res = BrowserInterop.vtsNavigationGetSubjective(Handle);
            Util.CheckInterop();
            return res;
        }

        public double[] GetPoint()
        {
            double[] res = new double[3];
            BrowserInterop.vtsNavigationGetPoint(Handle, res);
            Util.CheckInterop();
            return res;
        }

        public double[] GetRotation()
        {
            double[] res = new double[3];
            BrowserInterop.vtsNavigationGetRotation(Handle, res);
            Util.CheckInterop();
            return res;
        }

        public double[] GetRotationLimited()
        {
            double[] res = new double[3];
            BrowserInterop.vtsNavigationGetRotationLimited(Handle, res);
            Util.CheckInterop();
            return res;
        }

        public double GetViewExtent()
        {
            double res = BrowserInterop.vtsNavigationGetViewExtent(Handle);
            Util.CheckInterop();
            return res;
        }

        public double GetFov()
        {
            double res = BrowserInterop.vtsNavigationGetFov(Handle);
            Util.CheckInterop();
            return res;
        }

        public string GetPositionJson()
        {
            return Util.CheckString(BrowserInterop.vtsNavigationGetPositionJson(Handle));
        }

        public string GetPositionUrl()
        {
            return Util.CheckString(BrowserInterop.vtsNavigationGetPositionUrl(Handle));
        }
        
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~Navigation()
        {
            Dispose(false);
        }

        protected void Dispose(bool disposing)
        {
            if (Handle != IntPtr.Zero)
            {
                BrowserInterop.vtsNavigationDestroy(Handle);
                Util.CheckInterop();
                handle = IntPtr.Zero;
            }
        }

        protected IntPtr handle;
        public IntPtr Handle { get { return handle; } }
    }
}
