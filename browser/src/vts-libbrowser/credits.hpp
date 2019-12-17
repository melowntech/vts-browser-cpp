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

#ifndef CREDITS_HPP_edfgvbbnk
#define CREDITS_HPP_edfgvbbnk

#include <vts-libs/registry/referenceframe.hpp>

#include "include/vts-browser/cameraCredits.hpp"

namespace vtslibs { namespace vts {
struct Mapconfig;
} }

namespace vts
{

class Credits
{
public:
    enum class Scope
    {
        Imagery,
        Geodata,

        Total_
    };

    boost::optional<vtslibs::registry::CreditId> find(
            const std::string &name) const;
    void hit(Scope scope, vtslibs::registry::CreditId id, uint32 lod);
    std::string findId(vtslibs::registry::CreditId id) const;
    void tick(CameraCredits &credits);
    void merge(vtslibs::registry::RegistryBase *reg);
    void merge(vtslibs::registry::Credit credit);
    void purge();

private:
    vtslibs::registry::Credit::dict stor;
    
    struct Hit
    {
        Hit(vtslibs::registry::CreditId id);

        vtslibs::registry::CreditId id;
        uint32 hits;
        uint32 maxLod;
    };
    std::vector<Hit> hits[(int)Scope::Total_];
};

} // namespace vts

#endif
