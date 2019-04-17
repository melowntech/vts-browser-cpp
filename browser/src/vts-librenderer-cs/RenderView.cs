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
    public class RenderView : IDisposable
    {
        public static void LoadGlFunctions(RendererInterop.GLADloadproc proc)
        {
            RendererInterop.vtsLoadGlFunctions(proc);
            Util.CheckInterop();
        }

        public RenderView(RenderContext ctx, Camera camera)
        {
            Handle = RendererInterop.vtsRenderContextCreateView(ctx.Handle, camera.Handle);
            Util.CheckInterop();
        }

        public void Render()
        {
            RendererInterop.vtsRenderViewRender(Handle);
            Util.CheckInterop();
        }

        public void RenderCompass(double[] screenPosSize, double[] mapRotation)
        {
            Util.CheckArray(screenPosSize, 3);
            Util.CheckArray(mapRotation, 3);
            RendererInterop.vtsRenderViewRenderCompas(Handle, screenPosSize, mapRotation);
            Util.CheckInterop();
        }

        public double[] GetWorldPosition(double[] screenPos)
        {
            Util.CheckArray(screenPos, 2);
            double[] res = new double[3];
            RendererInterop.vtsRenderViewGetWorldPosition(Handle, screenPos, res);
            Util.CheckInterop();
            return res;
        }

        public RenderOptions Options
        {
            get
            {
                IntPtr p = RendererInterop.vtsRenderViewOptions(Handle);
                Util.CheckInterop();
                return (RenderOptions)Marshal.PtrToStructure(p, typeof(RenderOptions));
            }
            set
            {
                IntPtr p = RendererInterop.vtsRenderViewOptions(Handle);
                Util.CheckInterop();
                Marshal.StructureToPtr(value, p, false);
            }
        }

        public RenderVariables Variables
        {
            get
            {
                IntPtr p = RendererInterop.vtsRenderViewVariables(Handle);
                Util.CheckInterop();
                return (RenderVariables)Marshal.PtrToStructure(p, typeof(RenderVariables));
            }
        }

        public void Dispose()
        {
            RendererInterop.vtsRenderViewDestroy(Handle);
            Util.CheckInterop();
        }

        public IntPtr Handle { get; }
    }
}
