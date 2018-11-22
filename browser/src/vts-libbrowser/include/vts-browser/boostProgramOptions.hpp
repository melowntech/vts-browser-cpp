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

#ifndef BOOSTPROGRAMOPTIONS_HPP_qwsdfgzuweuiw4
#define BOOSTPROGRAMOPTIONS_HPP_qwsdfgzuweuiw4

#include <boost/program_options.hpp>
#include "foundation.hpp"

namespace vts
{

VTS_API void optionsConfigLog(
        boost::program_options::options_description &desc,
        std::string section = "log");

VTS_API void optionsConfigMapCreate(
        boost::program_options::options_description &desc,
        class MapCreateOptions *opts,
        std::string section = "map");

VTS_API void optionsConfigMapRuntime(
        boost::program_options::options_description &desc,
        class MapRuntimeOptions *opts,
        std::string section = "map");

VTS_API void optionsConfigCamera(
        boost::program_options::options_description &desc,
        class CameraOptions *opts,
        std::string section = "camera");

VTS_API void optionsConfigNavigation(
        boost::program_options::options_description &desc,
        class NavigationOptions *opts,
        std::string section = "navigation");

VTS_API void optionsConfigFetcherOptions(
        boost::program_options::options_description &desc,
        class FetcherOptions *opts,
        std::string section = "fetcher");

} // namespace vts

#endif
