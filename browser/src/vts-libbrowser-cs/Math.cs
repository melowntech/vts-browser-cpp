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
    public static class Math
    {
        public static double[] Mul44x44(double[] l, double[] r)
        {
            Util.CheckArray(l, 16);
            Util.CheckArray(r, 16);
            double[] res = new double[16];
            BrowserInterop.vtsMathMul44x44(res, l, r);
            Util.CheckInterop();
            return res;
        }

        public static double[] Mul33x33(double[] l, double[] r)
        {
            Util.CheckArray(l, 9);
            Util.CheckArray(r, 9);
            double[] res = new double[9];
            BrowserInterop.vtsMathMul33x33(res, l, r);
            Util.CheckInterop();
            return res;
        }

        public static double[] Mul44x4(double[] l, double[] r)
        {
            Util.CheckArray(l, 16);
            Util.CheckArray(r, 4);
            double[] res = new double[4];
            BrowserInterop.vtsMathMul44x4(res, l, r);
            Util.CheckInterop();
            return res;
        }

        public static double[] Inverse44(double[] r)
        {
            Util.CheckArray(r, 16);
            double[] res = new double[16];
            BrowserInterop.vtsMathInverse44(res, r);
            Util.CheckInterop();
            return res;
        }

        public static double[] Inverse33(double[] r)
        {
            Util.CheckArray(r, 9);
            double[] res = new double[9];
            BrowserInterop.vtsMathInverse33(res, r);
            Util.CheckInterop();
            return res;
        }

        public static double[] Normalize4(double[] r)
        {
            Util.CheckArray(r, 4);
            double len = System.Math.Sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3]);
            return new double[4] { r[0] / len, r[1] / len, r[2] / len, r[3] / len };
        }

        public static double[] Normalize3(double[] r)
        {
            Util.CheckArray(r, 3);
            double len = System.Math.Sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
            return new double[3] { r[0] / len, r[1] / len, r[2] / len };
        }
    }
}
