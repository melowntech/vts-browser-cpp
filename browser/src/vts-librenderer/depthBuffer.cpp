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
    : w{ 0,0 }, h{ 0,0 }, pbo{ 0,0 },
    tw(0), th(0), fbo(0), tex(0), index(0)
{
    glGenBuffers(PboCount, pbo);
    glGenTextures(1, &tex);
    glGenFramebuffers(1, &fbo);
}

DepthBuffer::~DepthBuffer()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    glDeleteBuffers(PboCount, pbo);
}

void DepthBuffer::performCopy(uint32 sourceTexture,
    uint32 paramW, uint32 paramH)
{
    paramW /= 3;
    paramH /= 3;
    glViewport(0, 0, paramW, paramH);

    // copy depth to texture (perform conversion)

    glBindTexture(GL_TEXTURE_2D, tex);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    if (tw != paramW || th != paramH)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, paramW, paramH, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, tex, 0);
        checkGlFramebuffer(GL_FRAMEBUFFER);

        CHECK_GL("read the depth (resize fbo and texture)");

        tw = paramW;
        th = paramH;
    }

    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    shaderCopyDepth->bind();
    meshQuad->bind();
    meshQuad->dispatch();

    CHECK_GL("read the depth (conversion)");

    // copy texture to pbo

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[index]);
    if (w[index] != paramW || h[index] != paramH)
    {
        glBufferData(GL_PIXEL_PACK_BUFFER,
            paramW * paramH * sizeof(float),
            nullptr, GL_STATIC_READ);
        w[index] = paramW;
        h[index] = paramH;
    }

    glReadPixels(0, 0, w[index], h[index],
        GL_RGBA, GL_UNSIGNED_BYTE, 0);

    CHECK_GL("read the depth (texture to pbo)");

    // copy gpu pbo to cpu buffer

    index = (index + 1) % PboCount;

    uint32 *depths = nullptr;
    {
        uint32 reqsiz = w[index] * h[index] * sizeof(uint32);
        if (buffer.size() < reqsiz)
            buffer.allocate(reqsiz);
        depths = (uint32*)buffer.data();
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[index]);
    glGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0,
        w[index] * h[index] * sizeof(uint32), depths);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    CHECK_GL("read the depth (pbo to cpu)");
}

float DepthBuffer::valuePix(uint32 x, uint32 y)
{
    if (w[index] * h[index] == 0)
        return nan1();
    assert(x < w[index] && y < h[index]);

    // convert uint32 to float depth
    union U
    {
        uint32 u;
        float f;
        unsigned char c[4];
    } u;
    u.f = ((float*)buffer.data())[x + y * w[index]];
    if (u.u == 0)
        return nan1();
    static const vec4 bitSh = vec4(
        1.0 / (256.0*256.0*256.0),
        1.0 / (256.0*256.0),
        1.0 / 256.0, 1.0);
    float d = 0;
    for (int i = 0; i < 4; i++)
        d += u.c[i] * bitSh[i];
    return d / 255.f;
}

float DepthBuffer::value(float x, float y)
{
    assert(x >= -1 && x <= 1 && y >= -1 && y <= 1);
    return valuePix((x * 0.5 + 0.5) * (w[index] - 1),
        (y * 0.5 + 0.5) * (h[index] - 1));
}

} } // namespace vts renderer

