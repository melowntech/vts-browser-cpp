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

#ifndef CELESTIAL_H_wegsfhnmo
#define CELESTIAL_H_wegsfhnmo

#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

VTS_API const char *vtsCelestialName(vtsHMap map);
VTS_API double vtsCelestialMajorRadius(vtsHMap map);
VTS_API double vtsCelestialMinorRadius(vtsHMap map);

// colors: horizon rgba, zenith rgba
// parameters: color gradient exponent,
//   thickness, quantile, visibility, quantile
VTS_API void vtsCelestialAtmosphere(vtsHMap map,
                float colors[8], double parameters[5]);
VTS_API void vtsCelestialAtmosphereDerivedAttributes(vtsHMap map,
                double *boundaryThickness,
                double *horizontalExponent, double *verticalExponent);

#ifdef __cplusplus
} // extern C
#endif

#endif
