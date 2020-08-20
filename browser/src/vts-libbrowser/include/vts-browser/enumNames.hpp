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

#ifndef ENUMNAMES_HPP_kjhgefs4
#define ENUMNAMES_HPP_kjhgefs4

#include "foundation.hpp"

namespace vts
{

#ifndef UTILITY_GENERATE_ENUM_IO
#include <boost/preprocessor/seq.hpp>
#define UTILITY_GENERATE_ENUM_IO_IT(R, DATA, ELEM) BOOST_PP_SEQ_ELEM(1, ELEM) ,
#define UTILITY_GENERATE_ENUM_IO(NAME, PAIRS) \
static constexpr const char *BOOST_PP_CAT(NAME, Names)[] = { \
    BOOST_PP_SEQ_FOR_EACH(UTILITY_GENERATE_ENUM_IO_IT, _, PAIRS) \
};
#define UNDEF_UTILITY_GENERATE_ENUM_IO
#endif

UTILITY_GENERATE_ENUM_IO(Srs,
    ((Physical)("physical"))
    ((Navigation)("navigation"))
    ((Public)("public"))
    ((Search)("search"))
    ((Custom1)("custom1"))
    ((Custom2)("custom2"))
)

UTILITY_GENERATE_ENUM_IO(NavigationType,
    ((Instant)("instant"))
    ((Quick)("quick"))
    ((FlyOver)("flyOver"))
)

UTILITY_GENERATE_ENUM_IO(NavigationMode,
    ((Azimuthal)("azimuthal"))
    ((Free)("free"))
    ((Dynamic)("dynamic"))
    ((Seamless)("seamless"))
)

UTILITY_GENERATE_ENUM_IO(TraverseMode,
    ((None)("none"))
    ((Flat)("flat"))
    ((Stable)("stable"))
    ((Balanced)("balanced"))
    ((Hierarchical)("hierarchical"))
    ((Fixed)("fixed"))
)

#ifdef UNDEF_UTILITY_GENERATE_ENUM_IO
#undef UTILITY_GENERATE_ENUM_IO_IT
#undef UTILITY_GENERATE_ENUM_IO
#endif

} // namespace vts

#endif

