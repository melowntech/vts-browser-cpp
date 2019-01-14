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
    public class Camera : IDisposable
    {
        public Camera(Map map)
        {
            handle = BrowserInterop.vtsCameraCreate(map.Handle);
            Util.CheckInterop();
        }

        public void SetViewportSize(uint width, uint height)
        {
            BrowserInterop.vtsCameraSetViewportSize(Handle, width, height);
            Util.CheckInterop();
        }
        
        public void SetView(double[] eye, double[] target, double[] up)
        {
            Util.CheckArray(eye, 3);
            Util.CheckArray(target, 3);
            Util.CheckArray(up, 3);
            BrowserInterop.vtsCameraSetView(Handle, eye, target, up);
            Util.CheckInterop();
        }

        public void SetView(double[] view)
        {
            Util.CheckArray(view, 16);
            BrowserInterop.vtsCameraSetViewMatrix(Handle, view);
            Util.CheckInterop();
        }

        public void SetProj(double fovyDegs, double near, double far)
        {
            BrowserInterop.vtsCameraSetProj(Handle, fovyDegs, near, far);
            Util.CheckInterop();
        }

        public void SetProj(double[] proj)
        {
            Util.CheckArray(proj, 16);
            BrowserInterop.vtsCameraSetProjMatrix(Handle, proj);
            Util.CheckInterop();
        }

        public void GetViewportSize(out uint width, out uint height)
        {
            width = height = 0;
            BrowserInterop.vtsCameraGetViewportSize(Handle, ref width, ref height);
            Util.CheckInterop();
        }

        public void GetView(out double[] eye, out double[] target, out double[] up)
        {
            eye = new double[3];
            target = new double[3];
            up = new double[3];
            BrowserInterop.vtsCameraGetView(Handle, eye, target, up);
            Util.CheckInterop();
        }

        public double[] GetView()
        {
            double[] view = new double[16];
            BrowserInterop.vtsCameraGetViewMatrix(Handle, view);
            Util.CheckInterop();
            return view;
        }

        public double[] GetProj()
        {
            double[] proj = new double[16];
            BrowserInterop.vtsCameraGetProjMatrix(Handle, proj);
            Util.CheckInterop();
            return proj;
        }

        public void SuggestedNearFar(out double near, out double far)
        {
            near = far = 0;
            BrowserInterop.vtsCameraSuggestedNearFar(Handle, ref near, ref far);
            Util.CheckInterop();
        }

        public string GetOptions()
        {
            return Util.CheckString(BrowserInterop.vtsCameraGetOptions(Handle));
        }

        public string GetStatistics()
        {
            return Util.CheckString(BrowserInterop.vtsCameraGetStatistics(Handle));
        }

        public string GetCredits()
        {
            return Util.CheckString(BrowserInterop.vtsCameraGetCredits(Handle));
        }

        public string GetCreditsShort()
        {
            return Util.CheckString(BrowserInterop.vtsCameraGetCreditsShort(Handle));
        }

        public string GetCreditsFull()
        {
            return Util.CheckString(BrowserInterop.vtsCameraGetCreditsFull(Handle));
        }

        public void SetOptions(string json)
        {
            BrowserInterop.vtsCameraSetOptions(Handle, json);
            Util.CheckInterop();
        }

        public void RenderUpdate()
        {
            BrowserInterop.vtsCameraRenderUpdate(Handle);
            Util.CheckInterop();
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~Camera()
        {
            Dispose(false);
        }

        protected void Dispose(bool disposing)
        {
            if (Handle != IntPtr.Zero)
            {
                BrowserInterop.vtsCameraDestroy(Handle);
                Util.CheckInterop();
                handle = IntPtr.Zero;
            }
        }

        protected IntPtr handle;
        public IntPtr Handle { get { return handle; } }
    }
}
