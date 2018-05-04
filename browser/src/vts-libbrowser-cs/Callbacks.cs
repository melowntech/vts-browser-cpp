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
        private void AssignInternalDelegates()
        {
            MapconfigAvailableDelegate = new BrowserInterop.vtsStateCallbackType(MapconfigAvailableCallback);
            MapconfigReadyDelegate = new BrowserInterop.vtsStateCallbackType(MapconfigReadyCallback);
            CameraEyeDelegate = new BrowserInterop.vtsCameraOverrideCallbackType(CameraEyeCallback);
            CameraTargetDelegate = new BrowserInterop.vtsCameraOverrideCallbackType(CameraTargetCallback);
            CameraUpDelegate = new BrowserInterop.vtsCameraOverrideCallbackType(CameraUpCallback);
            CameraFovAspectNearFarDelegate = new BrowserInterop.vtsCameraOverrideCallbackType(CameraFovAspectNearFarCallback);
            CameraViewDelegate = new BrowserInterop.vtsCameraOverrideCallbackType(CameraViewCallback);
            CameraProjDelegate = new BrowserInterop.vtsCameraOverrideCallbackType(CameraProjCallback);
            LoadTextureDelegate = new BrowserInterop.vtsResourceCallbackType(LoadTextureCallback);
            LoadMeshDelegate = new BrowserInterop.vtsResourceCallbackType(LoadMeshCallback);
            UnloadResourceDelegate = new BrowserInterop.vtsResourceDeleterCallbackType(UnloadResourceCallback);
        }

        public void AssignInternalCallbacks()
        {
            BrowserInterop.vtsCallbacksMapconfigAvailable(Handle, MapconfigAvailableDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksMapconfigReady(Handle, MapconfigReadyDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksCameraEye(Handle, CameraEyeDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksCameraTarget(Handle, CameraTargetDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksCameraUp(Handle, CameraUpDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksCameraFovAspectNearFar(Handle, CameraFovAspectNearFarDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksCameraView(Handle, CameraViewDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksCameraProj(Handle, CameraProjDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksLoadTexture(Handle, LoadTextureDelegate);
            Util.CheckError();
            BrowserInterop.vtsCallbacksLoadMesh(Handle, LoadMeshDelegate);
            Util.CheckError();
        }

        private BrowserInterop.vtsStateCallbackType MapconfigAvailableDelegate;
        private void MapconfigAvailableCallback(IntPtr h)
        {
            Debug.Assert(h == Handle);
            EventMapconfigAvailable?.Invoke();
        }

        private BrowserInterop.vtsStateCallbackType MapconfigReadyDelegate;
        private void MapconfigReadyCallback(IntPtr h)
        {
            Debug.Assert(h == Handle);
            EventMapconfigReady?.Invoke();
        }

        private BrowserInterop.vtsCameraOverrideCallbackType CameraEyeDelegate;
        private void CameraEyeCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraEye != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                EventCameraEye.Invoke(ref tmp);
                Util.CheckArray(tmp, 3);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private BrowserInterop.vtsCameraOverrideCallbackType CameraTargetDelegate;
        private void CameraTargetCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraTarget != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                EventCameraTarget.Invoke(ref tmp);
                Util.CheckArray(tmp, 3);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private BrowserInterop.vtsCameraOverrideCallbackType CameraUpDelegate;
        private void CameraUpCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraUp != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                EventCameraUp.Invoke(ref tmp);
                Util.CheckArray(tmp, 3);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private BrowserInterop.vtsCameraOverrideCallbackType CameraFovAspectNearFarDelegate;
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

        private BrowserInterop.vtsCameraOverrideCallbackType CameraViewDelegate;
        private void CameraViewCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraView != null)
            {
                double[] tmp = new double[16];
                Marshal.Copy(values, tmp, 0, 16);
                EventCameraView.Invoke(ref tmp);
                Util.CheckArray(tmp, 16);
                Marshal.Copy(tmp, 0, values, 16);
            }
        }

        private BrowserInterop.vtsCameraOverrideCallbackType CameraProjDelegate;
        private void CameraProjCallback(IntPtr h, IntPtr values)
        {
            Debug.Assert(h == Handle);
            if (EventCameraProj != null)
            {
                double[] tmp = new double[16];
                Marshal.Copy(values, tmp, 0, 16);
                EventCameraProj.Invoke(ref tmp);
                Util.CheckArray(tmp, 16);
                Marshal.Copy(tmp, 0, values, 16);
            }
        }

        private BrowserInterop.vtsResourceCallbackType LoadTextureDelegate;
        private void LoadTextureCallback(IntPtr h, IntPtr r)
        {
            Debug.Assert(h == Handle);
            if (EventLoadTexture != null)
            {
                Texture t = new Texture();
                t.Load(r);
                BrowserInterop.vtsSetResourceMemoryCost(r, 0, (uint)t.data.Length);
                Util.CheckError();
                GCHandle hnd = GCHandle.Alloc(EventLoadTexture.Invoke(t));
                BrowserInterop.vtsSetResourceUserData(r, GCHandle.ToIntPtr(hnd), UnloadResourceDelegate);
                Util.CheckError();
            }
        }

        private BrowserInterop.vtsResourceCallbackType LoadMeshDelegate;
        private void LoadMeshCallback(IntPtr h, IntPtr r)
        {
            Debug.Assert(h == Handle);
            if (EventLoadMesh != null)
            {
                Mesh m = new Mesh();
                m.Load(r);
                BrowserInterop.vtsSetResourceMemoryCost(r, 0, (uint)(m.vertices == null ? 0 : m.vertices.Length) + (uint)(m.indices == null ? 0 : m.indices.Length) * 2);
                Util.CheckError();
                GCHandle hnd = GCHandle.Alloc(EventLoadMesh.Invoke(m));
                BrowserInterop.vtsSetResourceUserData(r, GCHandle.ToIntPtr(hnd), UnloadResourceDelegate);
                Util.CheckError();
            }
        }

        private BrowserInterop.vtsResourceDeleterCallbackType UnloadResourceDelegate;
        private void UnloadResourceCallback(IntPtr ptr)
        {
            GCHandle hnd = GCHandle.FromIntPtr(ptr);
            hnd.Free();
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

        public delegate Object LoadTextureHandler(Texture texture);
        public event LoadTextureHandler EventLoadTexture;
        public delegate Object LoadMeshHandler(Mesh mesh);
        public event LoadMeshHandler EventLoadMesh;
    }
}
