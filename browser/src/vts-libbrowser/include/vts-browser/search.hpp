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

#ifndef SEARCH_HPP_gtvuigshefh
#define SEARCH_HPP_gtvuigshefh

#include <string>
#include <vector>
#include <memory>
#include <atomic>

#include "foundation.hpp"

namespace vts
{

class SearchTaskImpl;
class MapImpl;

class VTS_API SearchItem
{
public:
    SearchItem();
    explicit SearchItem(const std::string &json);

    std::string json;
    std::string id, type;
    std::string title, region;

    double position[3]; // navigation srs
    double distance; // physical srs
    double radius; // physical srs
};

class VTS_API SearchTask : private Immovable
{
public:
    explicit SearchTask(const std::string &query, const double point[3]);

    void updateDistances(const double point[3]); // navigation srs

    const std::string query;
    const double position[3];
    std::vector<SearchItem> results;
    std::atomic<bool> done; // do not access the results until this is true

private:
    std::shared_ptr<SearchTaskImpl> impl;
    friend MapImpl;
};

} // namespace vts

#endif
