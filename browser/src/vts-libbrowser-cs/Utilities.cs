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
    public enum LogLevel : uint
    {
        all =      0xfffffu,
        none =     0x00000u,
        debug =    0x0000fu,
        info1 =    0x000f0u,
        info2 =    0x00070u,
        info3 =    0x00030u,
        info4 =    0x00010u,
        warn1 =    0x00f00u,
        warn2 =    0x00700u,
        warn3 =    0x00300u,
        warn4 =    0x00100u,
        err1 =     0x0f000u,
        err2 =     0x07000u,
        err3 =     0x03000u,
        err4 =     0x01000u,
        fatal =    0xf0000u,
        default_ = info3 | warn2 | err2,
        verbose =  info2 | warn2 | err2,
        noDebug =  info1 | warn1 | err1,
    }

    public static class Util
    {
        public static void Log(LogLevel level, string message)
        {
            BrowserInterop.vtsLog((uint)level, message);
        }

        public static string CStringToSharp(IntPtr cstr)
        {
            return Marshal.PtrToStringAnsi(cstr);
        }

        public static void CheckArray(double[] arr, uint expected)
        {
            Debug.Assert(arr.Rank == 1 && arr.Length == expected);
        }

        public static void CheckArray(float[] arr, uint expected)
        {
            Debug.Assert(arr.Rank == 1 && arr.Length == expected);
        }

        public static void CheckInterop()
        {
            int code = BrowserInterop.vtsErrCode();
            if (code != 0)
            {
                string msg = CStringToSharp(BrowserInterop.vtsErrMsg());
                BrowserInterop.vtsErrClear();
                throw new VtsException(code, msg);
            }
        }

        public static string CheckString(IntPtr cstr)
        {
            CheckInterop();
            return CStringToSharp(cstr);
        }

        public static uint GpuTypeSize(GpuType type)
        {
            uint s = BrowserInterop.vtsGpuTypeSize((uint)type);
            CheckInterop();
            return s;
        }
    }
}
