
// Copyright (c) 2011-2012, Daniel M�ller <dm@g4t3.de>
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

#include "siderealtime.h"

#include "julianday.h"
#include "mathmacros.h"


namespace osgHimmel
{

const t_longf siderealTime(const t_aTime &aTime)
{
    const t_aTime gmt(makeUT(aTime));
    const t_julianDay JD(jdUT(gmt));

    // (AA.11.4)

    const t_longf T(jCenturiesSinceSE(JD));
    const t_longf t = 
        280.46061837 + 360.98564736629 * (jdSinceSE(JD))
        + T * T * (0.000387933 - T / 38710000.0);

    return _revd(t);
}


const t_longf siderealTime2(const t_aTime &aTime)
{
    const t_aTime gmt(makeUT(aTime));
    const t_julianDay JD(jdUT(aTime));

    // ("A Physically-Based Night Sky Model" - 2001 - Wann Jensen et al.)

    const float T(static_cast<float>(jCenturiesSinceSE(JD)));
    const float t = 4.894961f + 230121.675315f * T;

    return _revd(_deg(t));
}

} // namespace osgHimmel