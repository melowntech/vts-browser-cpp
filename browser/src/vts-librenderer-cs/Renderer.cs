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
using System.Runtime.InteropServices;

namespace vts
{
    public class Renderer : IDisposable
    {
        public static void LoadGlFunctions(RendererInterop.GLADloadproc proc)
        {
            RendererInterop.vtsLoadGlFunctions(proc);
            Util.CheckError();
        }

        public Renderer()
        {
            Handle = RendererInterop.vtsRendererCreate();
            Util.CheckError();
        }
        
        public void Initialize()
        {
            RendererInterop.vtsRendererInitialize(Handle);
            Util.CheckError();
        }

        public void Deinitialize()
        {
            RendererInterop.vtsRendererFinalize(Handle);
            Util.CheckError();
        }

        public void BindLoadFunctions(Map map)
        {
            RendererInterop.vtsRendererBindLoadFunctions(Handle, map.Handle);
            Util.CheckError();
        }

        public void Render(Map map)
        {
            RendererInterop.vtsRendererRender(Handle, map.Handle);
            Util.CheckError();
        }
        
        public void RenderCompass(double[] screenPosSize, double[] mapRotation)
        {
            Util.CheckArray(ref screenPosSize, 3);
            Util.CheckArray(ref mapRotation, 3);
            RendererInterop.vtsRendererRenderCompas(Handle, screenPosSize, mapRotation);
            Util.CheckError();
        }

        public double[] GetWorldPosition(double[] screenPos)
        {
            Util.CheckArray(ref screenPos, 2);
            double[] res = new double[3];
            RendererInterop.vtsRendererGetWorldPosition(Handle, screenPos, res);
            Util.CheckError();
            return res;
        }

        public RenderOptions Options
        {
            get
            {
                IntPtr p = RendererInterop.vtsRendererOptions(Handle);
                Util.CheckError();
                return (RenderOptions)Marshal.PtrToStructure(p, typeof(RenderOptions));
            }
            set
            {
                IntPtr p = RendererInterop.vtsRendererOptions(Handle);
                Util.CheckError();
                Marshal.StructureToPtr(value, p, false);
            }
        }

        public RenderVariables Variables
        {
            get
            {
                IntPtr p = RendererInterop.vtsRendererVariables(Handle);
                Util.CheckError();
                return (RenderVariables)Marshal.PtrToStructure(p, typeof(RenderVariables));
            }
        }

        public void Dispose()
        {
            RendererInterop.vtsRendererDestroy(Handle);
            Util.CheckError();
        }

        public IntPtr Handle { get; }
    }
}
