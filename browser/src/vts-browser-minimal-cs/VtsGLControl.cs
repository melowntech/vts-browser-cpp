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
using System.ComponentModel;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace vtsBrowserMinimalCs
{
    public partial class VtsGLControl : UserControl, ISupportInitialize, IDisposable
    {
        public VtsGLControl()
        {
            InitializeComponent();

            // override draw styles
            SetStyle(ControlStyles.AllPaintingInWmPaint, true);
            SetStyle(ControlStyles.UserPaint, true);
            SetStyle(ControlStyles.OptimizedDoubleBuffer, false);
        }

        protected override void OnPaintBackground(PaintEventArgs e)
        {
            // nothing
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            VtsGL.wglMakeCurrent(hDC, hRC);
            Draw?.Invoke(this, e);
            VtsGL.SwapBuffers(hDC);
        }

        void timerDrawing_Tick(object sender, EventArgs e)
        {
            Invalidate();
        }
        
        void ISupportInitialize.BeginInit()
        {
            // nothing
        }
        
        void ISupportInitialize.EndInit()
        {
            hDC = VtsGL.GetDC(Handle);

            // pixel format
            VtsGL.PIXELFORMATDESCRIPTOR pfd = new VtsGL.PIXELFORMATDESCRIPTOR();
            pfd.Init();
            pfd.nVersion = 1;
            pfd.dwFlags = 4 | 32 | 1;
            pfd.iPixelType = 0;
            pfd.cColorBits = (byte)32;
            pfd.cDepthBits = 0;
            pfd.cStencilBits = 0;
            pfd.iLayerType = 0;
            int iPixelformat;
            if ((iPixelformat = VtsGL.ChoosePixelFormat(hDC, pfd)) == 0)
                throw new Exception("Failed ChoosePixelFormat.");
            if (VtsGL.SetPixelFormat(hDC, iPixelformat, pfd) == 0)
                throw new Exception("Failed SetPixelFormat.");

            // basic render context
            hRC = VtsGL.wglCreateContext(hDC);
            VtsGL.wglMakeCurrent(hDC, hRC);

            // reinitialize context to modern version
            int[] attributes =
            {
                0x2091, 3, // major version
                0x2092, 3, // minor version
#if DEBUG
                0x2094, 0x0001, // debug context
#endif
                0x9126, 0x00000001, // core profile
                0
            };
            var hRC2 = VtsGL.CreateContextAttribsARB(hDC, IntPtr.Zero, attributes);
            VtsGL.wglMakeCurrent(IntPtr.Zero, IntPtr.Zero);
            VtsGL.wglDeleteContext(hRC);
            VtsGL.wglMakeCurrent(hDC, hRC2);
            hRC = hRC2;

            // event
            Init?.Invoke(this, new EventArgs());

            // update size
            OnSizeChanged(null);

            // timer
            timer.Interval = 16;
            timer.Enabled = true;
            timer.Tick += timerDrawing_Tick;
        }

        void IDisposable.Dispose()
        {
            VtsGL.wglMakeCurrent(hDC, hRC);
            Fin?.Invoke(this, new EventArgs());
            VtsGL.wglMakeCurrent(IntPtr.Zero, IntPtr.Zero);

            VtsGL.ReleaseDC(Handle, hDC);
            if (hRC != IntPtr.Zero)
            {
                VtsGL.wglDeleteContext(hRC);
                hRC = IntPtr.Zero;
            }
        }

        readonly Timer timer = new Timer();
        IntPtr hRC = IntPtr.Zero;
        IntPtr hDC = IntPtr.Zero;

        [Category("VtsGL")]
        public event DrawHandler Draw;
        public event DrawHandler Init;
        public event DrawHandler Fin;
    }

    public delegate void DrawHandler(object sender, EventArgs args);

    public static class VtsGL
    {
        private const string Kernel32 = "kernel32.dll";
        private const string Gdi32 = "gdi32.dll";
        private const string User32 = "user32.dll";
        private const string OpenGL32 = "opengl32.dll";

        private static IntPtr OpenGL32Module = LoadLibrary(OpenGL32);

        private static T GetDelegateFor<T>() where T : class
        {
            Type delegateType = typeof(T);
            string name = delegateType.Name;
            Delegate del = null;
            IntPtr proc = wglGetProcAddress(name);
            if (proc == IntPtr.Zero)
                throw new Exception("OpenGL extension function pointer for <" + name + "> was not found");
            del = Marshal.GetDelegateForFunctionPointer(proc, delegateType);
            return del as T;
        }

        public static IntPtr CreateContextAttribsARB(IntPtr hDC, IntPtr hShareContext, int[] attribList)
        {
            return GetDelegateFor<wglCreateContextAttribsARB>()(hDC, hShareContext, attribList);
        }

        private delegate IntPtr wglCreateContextAttribsARB(IntPtr hDC, IntPtr hShareContext, int[] attribList);

        public static IntPtr AnyGlFnc(string name)
        {
            IntPtr p = wglGetProcAddress(name);
            if (p == IntPtr.Zero)
                p = GetProcAddress(OpenGL32Module, name);
            return p;
        }

        [DllImport(Kernel32, SetLastError = true)] public static extern IntPtr LoadLibrary(string lpFileName);
        [DllImport(Kernel32, SetLastError = true)] public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);
        [DllImport(OpenGL32, SetLastError = true)] public static extern void glClear(uint mask);
        [DllImport(OpenGL32, SetLastError = true)] public static extern void glClearColor(float red, float green, float blue, float alpha);
        [DllImport(OpenGL32, SetLastError = true)] public static extern void glViewport(int x, int y, int width, int height);
        [DllImport(OpenGL32, SetLastError = true)] public static extern int wglMakeCurrent(IntPtr hdc, IntPtr hrc);
        [DllImport(OpenGL32, SetLastError = true)] public static extern IntPtr wglCreateContext(IntPtr hdc);
        [DllImport(OpenGL32, SetLastError = true)] public static extern int wglDeleteContext(IntPtr hrc);
        [DllImport(OpenGL32, SetLastError = true)] public static extern IntPtr wglGetProcAddress(string name);
        [DllImport(OpenGL32, SetLastError = true)] public static extern IntPtr wglGetCurrentContext();
        [DllImport(Gdi32, SetLastError = true)] public unsafe static extern int ChoosePixelFormat(IntPtr hDC, [In, MarshalAs(UnmanagedType.LPStruct)] PIXELFORMATDESCRIPTOR ppfd);
        [DllImport(Gdi32, SetLastError = true)] public unsafe static extern int SetPixelFormat(IntPtr hDC, int iPixelFormat, [In, MarshalAs(UnmanagedType.LPStruct)] PIXELFORMATDESCRIPTOR ppfd);
        [DllImport(Gdi32, SetLastError = true)] public static extern int SwapBuffers(IntPtr hDC);
        [DllImport(User32, SetLastError = true)] public static extern IntPtr GetDC(IntPtr hWnd);
        [DllImport(User32, SetLastError = true)] public static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);

        [StructLayout(LayoutKind.Explicit)]
        public class PIXELFORMATDESCRIPTOR
        {
            [FieldOffset(0)]
            public UInt16 nSize;
            [FieldOffset(2)]
            public UInt16 nVersion;
            [FieldOffset(4)]
            public UInt32 dwFlags;
            [FieldOffset(8)]
            public Byte iPixelType;
            [FieldOffset(9)]
            public Byte cColorBits;
            [FieldOffset(10)]
            public Byte cRedBits;
            [FieldOffset(11)]
            public Byte cRedShift;
            [FieldOffset(12)]
            public Byte cGreenBits;
            [FieldOffset(13)]
            public Byte cGreenShift;
            [FieldOffset(14)]
            public Byte cBlueBits;
            [FieldOffset(15)]
            public Byte cBlueShift;
            [FieldOffset(16)]
            public Byte cAlphaBits;
            [FieldOffset(17)]
            public Byte cAlphaShift;
            [FieldOffset(18)]
            public Byte cAccumBits;
            [FieldOffset(19)]
            public Byte cAccumRedBits;
            [FieldOffset(20)]
            public Byte cAccumGreenBits;
            [FieldOffset(21)]
            public Byte cAccumBlueBits;
            [FieldOffset(22)]
            public Byte cAccumAlphaBits;
            [FieldOffset(23)]
            public Byte cDepthBits;
            [FieldOffset(24)]
            public Byte cStencilBits;
            [FieldOffset(25)]
            public Byte cAuxBuffers;
            [FieldOffset(26)]
            public SByte iLayerType;
            [FieldOffset(27)]
            public Byte bReserved;
            [FieldOffset(28)]
            public UInt32 dwLayerMask;
            [FieldOffset(32)]
            public UInt32 dwVisibleMask;
            [FieldOffset(36)]
            public UInt32 dwDamageMask;

            public void Init()
            {
                nSize = (ushort)Marshal.SizeOf(this);
            }
        }
    }
}
