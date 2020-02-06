/**
 * Copyright (c) 2020 Melown Technologies SE
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

#include "common.hpp"

#include <chrono>

using TimerClock = std::chrono::high_resolution_clock;
using TimerPoint = std::chrono::time_point<TimerClock>;

inline TimerPoint now()
{
    return TimerClock::now();
}

struct DurationBuffer
{
    static const uint32 N = 60;
    float buffer[N];
    int index = 0;

    DurationBuffer()
    {
        for (auto &i : buffer)
            i = 0;
    }

    float avg() const
    {
        float sum = 0;
        for (float i : buffer)
            sum += i;
        return sum / N;
    }

    float max() const
    {
        float m = 0;
        for (float i : buffer)
            m = std::max(m, i);
        return m;
    }

    void update(float t)
    {
        buffer[index++ % N] = t;
    }

    void update(const TimerPoint &a, const TimerPoint &b)
    {
        update(std::chrono::duration_cast<
               std::chrono::microseconds>(b - a).count() / 1000.0);
    }
};

