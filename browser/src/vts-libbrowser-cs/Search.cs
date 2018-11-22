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
using System.Collections.Generic;

namespace vts
{
    public class SearchTask : IDisposable
    {
        public List<string> results = new List<string>();

        public SearchTask(IntPtr ptr)
        {
            this.ptr = ptr;
        }

        public bool Check()
        {
            bool res = BrowserInterop.vtsSearchGetDone(ptr);
            Util.CheckInterop();
            if (res)
            {
                results.Clear();
                uint cnt = BrowserInterop.vtsSearchGetResultsCount(ptr);
                for (uint i = 0; i < cnt; i++)
                {
                    string r = Util.CheckString(BrowserInterop.vtsSearchGetResultData(ptr, i));
                    results.Add(r);
                }
            }
            return res;
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~SearchTask()
        {
            Dispose(false);
        }

        protected void Dispose(bool disposing)
        {
            if (ptr != IntPtr.Zero)
            {
                BrowserInterop.vtsSearchDestroy(ptr);
                Util.CheckInterop();
                ptr = IntPtr.Zero;
            }
        }

        private IntPtr ptr;
    }
}
