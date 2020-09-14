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

#include <vts-browser/buffer.hpp>
#include <vts-browser/log.hpp>
#include <vts-browser/math.hpp>
#include <vts-browser/resources.hpp>
#include <vts-renderer/classes.hpp>

#include "mainWindow.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void MainWindow::makeScreenshot()
{
    glfwSetWindowTitle(window, "saving screenshot");
    const vts::renderer::RenderOptions &ro = view->options();
    const vts::renderer::RenderVariables &rv = view->variables();
    vts::GpuTextureSpec spec;
    spec.width = ro.width;
    spec.height = ro.height;
    spec.components = 3;
    spec.buffer.resize(spec.expectedSize());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, rv.frameReadBufferId);
    glReadPixels(0, 0, spec.width, spec.height, GL_RGB, GL_UNSIGNED_BYTE, spec.buffer.data());
    spec.verticalFlip();
    vts::Buffer b = spec.encodePng();
    vts::writeLocalFileBuffer("screenshot.png", b);
    glfwSetWindowTitle(window, "screenshot done");
}
