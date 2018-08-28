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

#if ENABLE_IL2CPP
using AOT;
#endif

namespace vts
{
    public partial class Map
    {
        private void AssignInternalDelegates()
        {
            MapconfigAvailableDelegate = new BrowserInterop.vtsEmptyCallbackType(MapconfigAvailableCallback);
            MapconfigReadyDelegate = new BrowserInterop.vtsEmptyCallbackType(MapconfigReadyCallback);
            CameraEyeDelegate = new BrowserInterop.vtsDoubleArrayCallbackType(CameraEyeCallback);
            CameraTargetDelegate = new BrowserInterop.vtsDoubleArrayCallbackType(CameraTargetCallback);
            CameraUpDelegate = new BrowserInterop.vtsDoubleArrayCallbackType(CameraUpCallback);
            CameraFovAspectNearFarDelegate = new BrowserInterop.vtsDoubleArrayCallbackType(CameraFovAspectNearFarCallback);
            CameraViewDelegate = new BrowserInterop.vtsDoubleArrayCallbackType(CameraViewCallback);
            CameraProjDelegate = new BrowserInterop.vtsDoubleArrayCallbackType(CameraProjCallback);
            CollidersCenterDelegate = new BrowserInterop.vtsDoubleArrayCallbackType(CollidersCenterCallback);
            CollidersDistanceDelegate = new BrowserInterop.vtsDoubleCallbackType(CollidersDistanceCallback);
            CollidersLodDelegate = new BrowserInterop.vtsUint32CallbackType(CollidersLodCallback);
            LoadTextureDelegate = new BrowserInterop.vtsResourceCallbackType(LoadTextureCallback);
            LoadMeshDelegate = new BrowserInterop.vtsResourceCallbackType(LoadMeshCallback);
            UnloadResourceDelegate = new BrowserInterop.vtsResourceDeleterCallbackType(UnloadResourceCallback);
        }

        public void AssignInternalCallbacks()
        {
            BrowserInterop.vtsCallbacksMapconfigAvailable(Handle, MapconfigAvailableDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksMapconfigReady(Handle, MapconfigReadyDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCameraEye(Handle, CameraEyeDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCameraTarget(Handle, CameraTargetDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCameraUp(Handle, CameraUpDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCameraFovAspectNearFar(Handle, CameraFovAspectNearFarDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCameraView(Handle, CameraViewDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCameraProj(Handle, CameraProjDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCollidersCenter(Handle, CollidersCenterDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCollidersDistance(Handle, CollidersDistanceDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksCollidersLod(Handle, CollidersLodDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksLoadTexture(Handle, LoadTextureDelegate);
            Util.CheckInterop();
            BrowserInterop.vtsCallbacksLoadMesh(Handle, LoadMeshDelegate);
            Util.CheckInterop();
        }

        private BrowserInterop.vtsEmptyCallbackType MapconfigAvailableDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsEmptyCallbackType))] static
#endif
        private void MapconfigAvailableCallback(IntPtr h)
        {
            Map m = GetMap(h);
            if (m.EventMapconfigAvailable != null)
                m.EventMapconfigAvailable.Invoke();
        }

        private BrowserInterop.vtsEmptyCallbackType MapconfigReadyDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsEmptyCallbackType))] static
#endif
        private void MapconfigReadyCallback(IntPtr h)
        {
            Map m = GetMap(h);
            if (m.EventMapconfigReady != null)
                m.EventMapconfigReady.Invoke();
        }

        private BrowserInterop.vtsDoubleArrayCallbackType CameraEyeDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleArrayCallbackType))] static
#endif
        private void CameraEyeCallback(IntPtr h, IntPtr values)
        {
            Map m = GetMap(h);
            if (m.EventCameraEye != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                m.EventCameraEye.Invoke(ref tmp);
                Util.CheckArray(tmp, 3);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private BrowserInterop.vtsDoubleArrayCallbackType CameraTargetDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleArrayCallbackType))] static
#endif
        private void CameraTargetCallback(IntPtr h, IntPtr values)
        {
            Map m = GetMap(h);
            if (m.EventCameraTarget != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                m.EventCameraTarget.Invoke(ref tmp);
                Util.CheckArray(tmp, 3);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private BrowserInterop.vtsDoubleArrayCallbackType CameraUpDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleArrayCallbackType))] static
#endif
        private void CameraUpCallback(IntPtr h, IntPtr values)
        {
            Map m = GetMap(h);
            if (m.EventCameraUp != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                m.EventCameraUp.Invoke(ref tmp);
                Util.CheckArray(tmp, 3);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private BrowserInterop.vtsDoubleArrayCallbackType CameraFovAspectNearFarDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleArrayCallbackType))] static
#endif
        private void CameraFovAspectNearFarCallback(IntPtr h, IntPtr values)
        {
            Map m = GetMap(h);
            if (m.EventCameraFovAspectNearFar != null)
            {
                double[] tmp = new double[4];
                Marshal.Copy(values, tmp, 0, 4);
                m.EventCameraFovAspectNearFar.Invoke(ref tmp[0], ref tmp[1], ref tmp[2], ref tmp[3]);
                Marshal.Copy(tmp, 0, values, 4);
            }
        }

        private BrowserInterop.vtsDoubleArrayCallbackType CameraViewDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleArrayCallbackType))] static
#endif
        private void CameraViewCallback(IntPtr h, IntPtr values)
        {
            Map m = GetMap(h);
            if (m.EventCameraView != null)
            {
                double[] tmp = new double[16];
                Marshal.Copy(values, tmp, 0, 16);
                m.EventCameraView.Invoke(ref tmp);
                Util.CheckArray(tmp, 16);
                Marshal.Copy(tmp, 0, values, 16);
            }
        }

        private BrowserInterop.vtsDoubleArrayCallbackType CameraProjDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleArrayCallbackType))] static
#endif
        private void CameraProjCallback(IntPtr h, IntPtr values)
        {
            Map m = GetMap(h);
            if (m.EventCameraProj != null)
            {
                double[] tmp = new double[16];
                Marshal.Copy(values, tmp, 0, 16);
                m.EventCameraProj.Invoke(ref tmp);
                Util.CheckArray(tmp, 16);
                Marshal.Copy(tmp, 0, values, 16);
            }
        }

        private BrowserInterop.vtsDoubleArrayCallbackType CollidersCenterDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleArrayCallbackType))] static
#endif
        private void CollidersCenterCallback(IntPtr h, IntPtr values)
        {
            Map m = GetMap(h);
            if (m.EventCollidersCenter != null)
            {
                double[] tmp = new double[3];
                Marshal.Copy(values, tmp, 0, 3);
                m.EventCollidersCenter.Invoke(ref tmp);
                Util.CheckArray(tmp, 3);
                Marshal.Copy(tmp, 0, values, 3);
            }
        }

        private BrowserInterop.vtsDoubleCallbackType CollidersDistanceDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsDoubleCallbackType))] static
#endif
        private void CollidersDistanceCallback(IntPtr h, ref double value)
        {
            Map m = GetMap(h);
            if (m.EventCollidersDistance != null)
                m.EventCollidersDistance.Invoke(ref value);
        }

        private BrowserInterop.vtsUint32CallbackType CollidersLodDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsUint32CallbackType))] static
#endif
        private void CollidersLodCallback(IntPtr h, ref uint value)
        {
            Map m = GetMap(h);
            if (m.EventCollidersLod != null)
                m.EventCollidersLod.Invoke(ref value);
        }

        private BrowserInterop.vtsResourceCallbackType LoadTextureDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsResourceCallbackType))] static
#endif
        private void LoadTextureCallback(IntPtr h, IntPtr r)
        {
            Map m = GetMap(h);
            if (m.EventLoadTexture != null)
            {
                Texture t = new Texture();
                t.Load(r);
                BrowserInterop.vtsSetResourceMemoryCost(r, 0, (uint)t.data.Length);
                Util.CheckInterop();
                GCHandle hnd = GCHandle.Alloc(m.EventLoadTexture.Invoke(t));
                BrowserInterop.vtsSetResourceUserData(r, GCHandle.ToIntPtr(hnd), m.UnloadResourceDelegate);
                Util.CheckInterop();
            }
        }

        private BrowserInterop.vtsResourceCallbackType LoadMeshDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsResourceCallbackType))] static
#endif
        private void LoadMeshCallback(IntPtr h, IntPtr r)
        {
            Map m = GetMap(h);
            if (m.EventLoadMesh != null)
            {
                Mesh msh = new Mesh();
                msh.Load(r);
                BrowserInterop.vtsSetResourceMemoryCost(r, 0, (uint)(msh.vertices == null ? 0 : msh.vertices.Length) + (uint)(msh.indices == null ? 0 : msh.indices.Length) * 2);
                Util.CheckInterop();
                GCHandle hnd = GCHandle.Alloc(m.EventLoadMesh.Invoke(msh));
                BrowserInterop.vtsSetResourceUserData(r, GCHandle.ToIntPtr(hnd), m.UnloadResourceDelegate);
                Util.CheckInterop();
            }
        }

        private BrowserInterop.vtsResourceDeleterCallbackType UnloadResourceDelegate;
#if ENABLE_IL2CPP
        [MonoPInvokeCallback(typeof(BrowserInterop.vtsResourceDeleterCallbackType))] static
#endif
        private void UnloadResourceCallback(IntPtr ptr)
        {
            GCHandle hnd = GCHandle.FromIntPtr(ptr);
            object obj = hnd.Target;
            hnd.Free();
            IDisposable d = obj as IDisposable;
            if (d != null)
                d.Dispose();
        }

        public delegate void MapEmptyHandler();
        public delegate void DoubleArrayHandler(ref double[] values);
        public delegate void DoubleHandler(ref double value);
        public delegate void Uint32Handler(ref uint value);
        public delegate void CameraParamsHandler(ref double fov, ref double aspect, ref double near, ref double far);
        public delegate Object LoadTextureHandler(Texture texture);
        public delegate Object LoadMeshHandler(Mesh mesh);

        public event MapEmptyHandler EventMapconfigAvailable;
        public event MapEmptyHandler EventMapconfigReady;

        public event DoubleArrayHandler EventCameraEye;
        public event DoubleArrayHandler EventCameraTarget;
        public event DoubleArrayHandler EventCameraUp;
        public event CameraParamsHandler EventCameraFovAspectNearFar;
        public event DoubleArrayHandler EventCameraView;
        public event DoubleArrayHandler EventCameraProj;

        public event DoubleArrayHandler EventCollidersCenter;
        public event DoubleHandler EventCollidersDistance;
        public event Uint32Handler EventCollidersLod;

        public event LoadTextureHandler EventLoadTexture;
        public event LoadMeshHandler EventLoadMesh;
    }
}
