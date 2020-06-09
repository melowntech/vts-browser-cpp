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

#ifndef THREAD_QUEUE_gdf5g4d56f4ghd6h4
#define THREAD_QUEUE_gdf5g4d56f4ghd6h4

#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace vts
{

template<class T>
class ThreadQueue
{
public:
    void push(const T &v)
    {
        {
            std::lock_guard<std::mutex> lock(mut);
            if (stop)
                return;
            q.push_back(v);
        }
        con.notify_one();
    }

    void push(T &&v)
    {
        {
            std::lock_guard<std::mutex> lock(mut);
            if (stop)
                return;
            q.push_back(std::move(v));
        }
        con.notify_one();
    }

    bool tryPop(T &v)
    {
        std::lock_guard<std::mutex> lock(mut);
        if (q.empty() || stop)
            return false;
        v = std::move(q.front());
        q.erase(q.begin());
        return true;
    }

    bool waitPop(T &v)
    {
        std::unique_lock<std::mutex> lock(mut);
        while (q.empty() && !stop)
            con.wait(lock);
        if (q.empty() || stop)
            return false;
        v = std::move(q.front());
        q.erase(q.begin());
        return true;
    }

    std::vector<T> readAllWait()
    {
        std::unique_lock<std::mutex> lock(mut);
        if (stop)
            return {};
        con.wait(lock);
        if (stop)
            return {};
        std::vector<T> res;
        res.swap(q);
        return res;
    }

    void writeAll(std::vector<T> &writing)
    {
        {
            std::unique_lock<std::mutex> lock(mut);
            if (stop)
                return;
            writing.swap(q);
        }
        con.notify_one();
    }

    void terminate()
    {
        {
            std::lock_guard<std::mutex> lock(mut);
            stop = true;
        }
        con.notify_all();
    }

    void purge()
    {
        {
            std::lock_guard<std::mutex> lock(mut);
            stop = true;
            q.clear();
        }
        con.notify_all();
    }

    bool stopped() const
    {
        return stop;
    }

    uint32 estimateSize() const
    {
        return q.size();
    }

private:
    std::atomic<bool> stop {false};
    std::vector<T> q;
    mutable std::mutex mut;
    std::condition_variable con;
};

} // namespace vts

#endif
