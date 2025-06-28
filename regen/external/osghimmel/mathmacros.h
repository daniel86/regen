// Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
// Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the Computer Graphics Systems Group at the 
//     Hasso-Plattner-Institute (HPI), Germany nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.

#pragma once
#ifndef __MATHMACROS_H__
#define __MATHMACROS_H__

#include <cmath>
#include <regen/math/math.h>

namespace osgHimmel
{
#define _PI2  regen::math::twoPi<t_longf>()
#define _PI_2 regen::math::halfPi<t_longf>()

#define _abs(v) \
    (((v) < 0) ? -(v) : (v))

#define _int(v) \
    (static_cast<int>(v))

#define _short(v) \
    (static_cast<short>(v))

#define _mod(a, m) \
    ((a) - (m) * (_int((a) / (m)) - ((a) < 0 ? 1 : 0)))

#define _frac(x) \
    ((x) - _int(x))

#define _clamp(l, u, x) \
    ((x) < (l) ? (l) : (x) > (u) ? (u) : (x))

#define _deg(rad) ((rad) * regen::math::RAD_TO_DEG)

#define _rad(deg) ((deg) * regen::math::DEG_TO_RAD)

// same as _hour
// note: if d is negative use -_decimal(d, m, s) instead of _decimal(-d, m, s)
#define _decimal(d, m, s) \
    ((d) + ((m) + (s) / 60.0f) / 60.0f)

#define _adiameter(D, r) \
    (2 * atan(r / D))

// normalizes an angle to between 0 and 2PI radians
#define _rev(rad) \
    ((rad) - floor((rad) / _PI2) * _PI2)

// normalizes an angle to between 0 and 360 degrees
#define _revd(deg) \
    ((deg) - floor((deg) / 360.0L) * 360.0L)

#define _rightascd(deg, min, sec) \
    (_decimal(deg, min, sec) * 15.0L)

#define _day(h, m, s) \
    (_hour(h, m, s) / 24.0L)

// note: if h is negative use -_hour(h, m, s) instead of _hour(-h, m, s)
#define _hour(h, m, s) \
    ((h) + ((m) + (s) / 60.0L) / 60.0L)

#define _hours(d) \
    (_mod(d / 15.0L, 24.0L))

#define _kms(AUs) \
    (AUs * 149598000)

#define _randf(min, max) \
    (static_cast<float>(rand()) / RAND_MAX * ((max) - (min)) + (min))

#define _smootherstep(t) ((t) * (t) * (t) * ((t) * (6 * (t) - 15) + 10))

} // namespace osgHimmel

#endif // __MATHMACROS_H__
