
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

#include "sun.h"

#include "earth.h"
#include "moon.h"
#include "siderealtime.h"
#include "mathmacros.h"

using namespace osgHimmel;

float Sun::meanAnomaly(const t_julianDay &t) {
    const t_julianDay T(jCenturiesSinceSE(t));
    // ("A Physically-Based Night Sky Model" - 2001 - Wann Jensen et al.)
    const float M = _deg(6.24 + 628.302 * T);
    // suggestion (seems insuficient) from
    // http://wlym.com/~animations/ceres/calculatingposition/eccentric.html
    //const float M = 357.528 + 0.9856003 * t;
    return _revd(M);

}

float Sun::meanLongitude(const t_julianDay &t) {
    const t_julianDay T(jCenturiesSinceSE(t));
    const float L0 = 280.4665f + T * (+  36000.7698f);
    return _revd(L0);
}

t_equf Sun::apparentPosition(const t_julianDay &t) {
    // ("A Physically-Based Night Sky Model" - 2001 - Wann Jensen et al.)
    const t_julianDay T(jCenturiesSinceSE(t));
    const float M = _rad(meanAnomaly(t));

    t_eclf ecl;
    ecl.longitude = _deg(4.895048 + 628.331951 * T
        + (0.033417 - 0.000084 * T) * sin(M) + 0.000351 * sin(2 * M));
    ecl.latitude = 0;

    t_equf equ;
    equ = ecl.toEquatorial(Earth::trueObliquity(t));
    return equ;
}

t_horf Sun::horizontalPosition(const t_aTime &aTime, float latitude, float longitude) {
    t_julianDay t(jd(aTime));
    t_julianDay s(siderealTime(aTime));
    t_equf equ = apparentPosition(t);
    return equ.toHorizontal(s, latitude, longitude);
}

float Sun::distance(const t_julianDay &t) {
    // ("A Physically-Based Night Sky Model" - 2001 - Wann Jensen et al.)
    const t_julianDay T(jCenturiesSinceSE(t));
    const float M = 6.24f + 628.302f * T;
    const float R = 1.000140f - (0.016708f - 0.000042f * T) * cos(M) - 0.000141f * cos(2 * M); // in AU
    return _kms(R);
}

float Sun::meanRadius() {
    // http://nssdc.gsfc.nasa.gov/planetary/factsheet/sunfact.html
    return 0.696e+6; // in kilometers
}
