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

#include "renderer.hpp"

namespace vts { namespace renderer
{

DepthBuffer::DepthBuffer()
    : index(0), pbo{ 0, 0 }, w{ 0, 0 }, h{ 0, 0 }
#ifdef VTSR_OPENGLES
    , esFbo(0)
#endif
{
    glGenBuffers(PboCount, pbo);

#ifdef VTSR_OPENGLES
    esTex;
    esFbo;
#endif
}

DepthBuffer::~DepthBuffer()
{
    glDeleteBuffers(PboCount, pbo);

#ifdef VTSR_OPENGLES
    esFbo;
#endif
}

void DepthBuffer::performCopy(uint32 fbo, uint32 paramW, uint32 paramH)
{
    // copy framebuffer to pbo

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[index]);
    if (w[index] != paramW || h[index] != paramH)
    {
        glBufferData(GL_PIXEL_PACK_BUFFER,
            paramW * paramH * sizeof(float),
            nullptr, GL_STREAM_READ);
        w[index] = paramW;
        h[index] = paramH;
    }

#ifdef VTSR_OPENGLES
    // opengl ES does not support reading depth with glReadPixels

    // todo

#else

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
    glReadPixels(0, 0, w[index], h[index],
        GL_DEPTH_COMPONENT, GL_FLOAT, 0);

#endif

    CHECK_GL("read the depth (framebuffer to pbo)");

    // copy gpu pbo to cpu buffer
    index = (index + 1) % PboCount;

    float *depths = nullptr;
    {
        uint32 reqsiz = w[index] * h[index] * sizeof(float);
        if (buffer.size() < reqsiz)
            buffer.allocate(reqsiz);
        depths = (float*)buffer.data();
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[index]);
    glGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0,
        w[index] * h[index] * sizeof(float), depths);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    CHECK_GL("read the depth (pbo to cpu)");
}

float DepthBuffer::valuePix(uint32 x, uint32 y)
{
    if (w[index] * h[index] == 0)
        return nan1();
    assert(x < w[index] && y < h[index]);
    return ((float*)buffer.data())[x + y * w[index]];
}

float DepthBuffer::valueNdc(float x, float y)
{
    assert(x >= -1 && x <= 1 && y >= -1 && y <= 1);
    return valuePix((x * 0.5 + 0.5) * (w[index] - 1),
        (y * 0.5 + 0.5) * (h[index] - 1));
}

} } // namespace vts renderer

