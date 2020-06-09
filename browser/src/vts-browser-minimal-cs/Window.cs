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
using System.Windows.Forms;
using vts;

namespace vtsBrowserMinimalCs
{
    public partial class Window : Form
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Window());
        }

        public Window()
        {
            InitializeComponent();
            stopwatch = new System.Diagnostics.Stopwatch();
        }

        public void Init(object sender, EventArgs args)
        {
            RenderContext.LoadGlFunctions(VtsGL.AnyGlFnc);
            map = new Map("");
            cam = new Camera(map);
            nav = new Navigation(cam);
            renderContext = new RenderContext();
            renderView = new RenderView(renderContext, cam);
            map.SetMapconfigPath("https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");
            renderContext.BindLoadFunctions(map);
        }

        public void Fin(object sender, EventArgs args)
        {
            map.RenderFinalize();
            map.DataFinalize();
            nav = null;
            cam = null;
            map = null;
            renderView = null;
            renderContext = null;
        }

        public void Draw(object sender, EventArgs args)
        {
            var c = sender as VtsGLControl;
            if (c.Width <= 0 || c.Height <= 0)
                return;

            map.DataUpdate();
            long elapsed = stopwatch.ElapsedMilliseconds;
            stopwatch.Restart();
            map.RenderUpdate(elapsed / 1000.0);
            cam.SetViewportSize((uint)c.Width, (uint)c.Height);
            cam.RenderUpdate();

            RenderOptions ro = renderView.Options;
            ro.width = (uint)c.Width;
            ro.height = (uint)c.Height;
            renderView.Options = ro;
            renderView.Render();
        }

        public void MapMousePress(object sender, MouseEventArgs e)
        {
            LastMouseX = e.X;
            LastMouseY = e.Y;
        }

        public void MapMouseMove(object sender, MouseEventArgs e)
        {
            int xrel = e.X - LastMouseX;
            int yrel = e.Y - LastMouseY;
            LastMouseX = e.X;
            LastMouseY = e.Y;
            if (System.Math.Abs(xrel) > 100 || System.Math.Abs(yrel) > 100)
                return; // ignore the move event if it was too far away
            double[] p = new double[3];
            p[0] = xrel;
            p[1] = yrel;
            if (e.Button == MouseButtons.Left)
                nav.Pan(p);
            if (e.Button == MouseButtons.Right)
                nav.Rotate(p);
        }

        public void MapMouseWheel(object sender, MouseEventArgs e)
        {
            nav.Zoom(e.Delta / 120);
        }

        public int LastMouseX, LastMouseY;
        public Map map;
        public Camera cam;
        public Navigation nav;
        public RenderContext renderContext;
        public RenderView renderView;
        public System.Diagnostics.Stopwatch stopwatch;
    }
}
