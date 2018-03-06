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

#include <array>

namespace vts
{

template<class T, unsigned int N>
struct Array
{
private:
    typedef std::array<T, N> A;
    A a_;
    unsigned int s_;

public:
    typename A::iterator begin() { return a_.begin(); }
    typename A::iterator end() { return a_.begin() + s_; }
    Array() : s_(0) {}
    void clear() { resize(0); }
    void resize(unsigned int s)
    {
        if (s > N)
            throw std::runtime_error("overflow error");
        for (unsigned int i = std::min(s, s_); i < std::max(s, s_); i++)
            a_[i] = T();
        s_ = s;
    }
    void push_back(T &&v) { resize(s_ + 1); a_[s_ - 1] = std::move(v); }
    unsigned int size() const { return s_; }
    unsigned int capacity() const { return N; }
    bool empty() const { return s_ == 0; }
};

} // namespace vts


