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
                BrowserInterop.vtsResourceSetMemoryCost(r, 0, (uint)t.data.Length);
                Util.CheckInterop();
                GCHandle hnd = GCHandle.Alloc(m.EventLoadTexture.Invoke(t));
                BrowserInterop.vtsResourceSetUserData(r, GCHandle.ToIntPtr(hnd), m.UnloadResourceDelegate);
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
                BrowserInterop.vtsResourceSetMemoryCost(r, 0, (uint)(msh.vertices == null ? 0 : msh.vertices.Length) + (uint)(msh.indices == null ? 0 : msh.indices.Length) * 2);
                Util.CheckInterop();
                GCHandle hnd = GCHandle.Alloc(m.EventLoadMesh.Invoke(msh));
                BrowserInterop.vtsResourceSetUserData(r, GCHandle.ToIntPtr(hnd), m.UnloadResourceDelegate);
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
        public delegate Object LoadTextureHandler(Texture texture);
        public delegate Object LoadMeshHandler(Mesh mesh);

        public event MapEmptyHandler EventMapconfigAvailable;
        public event MapEmptyHandler EventMapconfigReady;

        public event LoadTextureHandler EventLoadTexture;
        public event LoadMeshHandler EventLoadMesh;
    }
}
