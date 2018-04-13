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

class VTS_API SearchItem
{
public:
    SearchItem();
    SearchItem(const std::string &json);
    std::string toJson() const;

    std::string displayName, title, type, region;
    std::string road, city, county, state, houseNumber,
                stateDistrict, country, countryCode;

    double position[3]; // navigation srs
    double radius; // physical srs length
    double distance; // physical srs length
    double importance;
};

class VTS_API SearchTask
{
public:
    SearchTask(const std::string &query, const double point[3]);
    virtual ~SearchTask();

    void updateDistances(const double point[3]); // navigation srs

    const std::string query;
    const double position[3];
    std::vector<SearchItem> results;
    std::atomic<bool> done;

    std::shared_ptr<class SearchTaskImpl> impl;
};

} // namespace vts

#endif
