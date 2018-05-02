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
using System.Runtime.InteropServices;

namespace vts
{
    public partial class Map
    {
        public void AssignInternalCallbacks()
        {
            BrowserInterop.vtsCallbacksMapconfigAvailable(Handle, MapconfigAvailableCallback);
            BrowserInterop.vtsCallbacksMapconfigReady(Handle, MapconfigReadyCallback);
            Util.CheckError();
            BrowserInterop.vtsCallbacksCameraEye(Handle, CameraEyeCallback);
            BrowserInterop.vtsCallbacksCameraTarget(Handle, CameraTargetCallback);
            BrowserInterop.vtsCallbacksCameraUp(Handle, CameraUpCallback);
            BrowserInterop.vtsCallbacksCameraFovAspectNearFar(Handle, CameraFovAspectNearFarCallback);
            BrowserInterop.vtsCallbacksCameraView(Handle, CameraViewCallback);
            BrowserInterop.vtsCallbacksCameraProj(Handle, CameraProjCallback);
            Util.CheckError();
            BrowserInterop.vtsCallbacksLoadTexture(Handle, LoadTextureCallback);
            BrowserInterop.vtsCallbacksLoadMesh(Handle, LoadMeshCallback);
            Util.CheckError();
        }

        private void MapconfigAvailableCallback(IntPtr h)
        {
            Debug.Assert(h == Handle);
            EventMapconfigAvailable?.Invoke();
        }

        private void MapconfigReadyCallback(IntPtr h)
        {
            Debug.Assert(h == Handle);
            EventMapconfigReady?.Invoke();
        }

        private void CameraEyeCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraEye != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                EventCameraEye.Invoke(ref tmp);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private void CameraTargetCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraTarget != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                EventCameraTarget.Invoke(ref tmp);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private void CameraUpCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraUp != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                EventCameraUp.Invoke(ref tmp);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private void CameraFovAspectNearFarCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraFovAspectNearFar != null)
            {
                double[] tmp = new double[4];
                Marshal.Copy(values, tmp, 0, 4);
                EventCameraFovAspectNearFar.Invoke(ref tmp[0], ref tmp[1], ref tmp[2], ref tmp[3]);
                Marshal.Copy(tmp, 0, values, 4);
            }
        }

        private void CameraViewCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraView != null)
            {
                double[] tmp = new double[16];
                Marshal.Copy(values, tmp, 0, 16);
                EventCameraView.Invoke(ref tmp);
                Marshal.Copy(tmp, 0, values, 16);
            }
        }

        private void CameraProjCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraProj != null)
            {
                double[] tmp = new double[16];
                Marshal.Copy(values, tmp, 0, 16);
                EventCameraProj.Invoke(ref tmp);
                Marshal.Copy(tmp, 0, values, 16);
            }
        }

        private void LoadTextureCallback(IntPtr h, IntPtr r)
        {
            Debug.Assert(h == Handle);
            if (EventLoadTexture != null)
            {
                Texture t = new Texture();
                t.Load(r);
                BrowserInterop.vtsSetResourceMemoryCost(r, 0, (uint)t.data.Length);
                Util.CheckError();
                IntPtr n = EventLoadTexture.Invoke(t);
                BrowserInterop.vtsSetResourceUserData(r, n, UnloadResourceCallback);
                Util.CheckError();
            }
        }

        private void LoadMeshCallback(IntPtr h, IntPtr r)
        {
            Debug.Assert(h == Handle);
            if (EventLoadMesh != null)
            {
                Mesh m = new Mesh();
                m.Load(r);
                BrowserInterop.vtsSetResourceMemoryCost(r, 0, (uint)(m.vertices == null ? 0 : m.vertices.Length) + (uint)(m.indices == null ? 0 : m.indices.Length) * 2);
                Util.CheckError();
                IntPtr n = EventLoadMesh.Invoke(m);
                BrowserInterop.vtsSetResourceUserData(r, n, UnloadResourceCallback);
                Util.CheckError();
            }
        }

        private void UnloadResourceCallback(IntPtr n)
        {
            EventUnloadResource?.Invoke(n);
        }

        public delegate void MapStateHandler();
        public event MapStateHandler EventMapconfigAvailable;
        public event MapStateHandler EventMapconfigReady;

        public delegate void CameraOverrideHandler(ref double[] values);
        public delegate void CameraOverrideParamsHandler(ref double fov, ref double aspect, ref double near, ref double far);
        public event CameraOverrideHandler EventCameraEye;
        public event CameraOverrideHandler EventCameraTarget;
        public event CameraOverrideHandler EventCameraUp;
        public event CameraOverrideParamsHandler EventCameraFovAspectNearFar;
        public event CameraOverrideHandler EventCameraView;
        public event CameraOverrideHandler EventCameraProj;

        public delegate IntPtr LoadTextureHandler(Texture texture);
        public event LoadTextureHandler EventLoadTexture;

        public delegate IntPtr LoadMeshHandler(Mesh mesh);
        public event LoadMeshHandler EventLoadMesh;

        public delegate void UnloadResourceHandler(IntPtr resource);
        public event UnloadResourceHandler EventUnloadResource;
    }
}
