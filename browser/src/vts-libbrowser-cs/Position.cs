/**
 * Copyright (c) 2019 Melown Technologies SE
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
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace vts
{
    [StructLayout(LayoutKind.Sequential)]
    public struct PositionBase
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public double[] point;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public double[] orientation;
        [MarshalAs(UnmanagedType.R8)] public double viewExtent;
        [MarshalAs(UnmanagedType.R8)] public double fov;
        [MarshalAs(UnmanagedType.I1)] public bool subjective;
        [MarshalAs(UnmanagedType.I1)] public bool floatAltitude;
    }

    public class Position
    {
        public PositionBase data;

        public static Position FromString(string str)
        {
            Position p = new Position();
            IntPtr i = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(PositionBase)));
            BrowserInterop.vtsPositionFromString(i, str);
            p.data = (PositionBase)Marshal.PtrToStructure(i, typeof(PositionBase));
            Marshal.FreeHGlobal(i);
            Util.CheckInterop();
            return p;
        }

        public string ToJson()
        {
            IntPtr i = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(PositionBase)));
            Marshal.StructureToPtr(data, i, true);
            IntPtr s = BrowserInterop.vtsPositionToJson(i);
            Marshal.FreeHGlobal(i);
            return Util.CheckString(s);
        }

        public string ToUrl()
        {
            IntPtr i = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(PositionBase)));
            Marshal.StructureToPtr(data, i, true);
            IntPtr s = BrowserInterop.vtsPositionToUrl(i);
            Marshal.FreeHGlobal(i);
            return Util.CheckString(s);
        }
    }
}
