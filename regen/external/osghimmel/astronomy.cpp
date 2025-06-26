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

#include "astronomy.h"

#include "earth.h"
#include "sun.h"
#include "moon.h"
#include "stars.h"
#include "siderealtime.h"

using namespace osgHimmel;

Astronomy::Astronomy() {
}

float Astronomy::sunDistance(const t_julianDay &t) const {
	return Sun::distance(t);
}

float Astronomy::angularSunRadius(const t_julianDay &t) const {
	return Earth::apparentAngularSunDiameter(t) * 0.5f;
}

float Astronomy::moonRadius() const {
	return Moon::meanRadius();
}

float Astronomy::moonDistance(const t_julianDay &t) const {
	return Moon::distance(t);
}

float Astronomy::angularMoonRadius(const t_julianDay &t) const {
	return Earth::apparentAngularMoonDiameter(t) * 0.5f;
}

regen::Vec3f Astronomy::moonPosition(const t_aTime &aTime, float latitude,
									 float longitude, bool refractionCorrected) const {
	t_horf moon = Moon::horizontalPosition(aTime, latitude, longitude);
	if (refractionCorrected)
		moon.altitude += Earth::atmosphericRefraction(moon.altitude);

	regen::Vec3f moon_v = moon.toEuclidean();
	moon_v.normalize();
	return moon_v;
}

regen::Vec3f Astronomy::sunPosition(const t_aTime &aTime, float latitude,
									float longitude, bool refractionCorrected) const {
	t_horf sun = Sun::horizontalPosition(aTime, latitude, longitude);
	if (refractionCorrected)
		sun.altitude += Earth::atmosphericRefraction(sun.altitude);

	regen::Vec3f sun_v = sun.toEuclidean();
	sun_v.normalize();
	return sun_v;
}

regen::Mat4f Astronomy::moonOrientation(const t_aTime &aTime, float latitude, float longitude) const {
	const t_julianDay t(jd(aTime));

	float l, b;
	Moon::opticalLibrations(t, l, b);

	const regen::Mat4f libLat = regen::Mat4f::rotationMatrix(-_rad(b), 0, 0);
	const regen::Mat4f libLon = regen::Mat4f::rotationMatrix(0, _rad(l), 0);

	const float a = _rad(Moon::positionAngleOfAxis(t));
	const float p = _rad(Moon::parallacticAngle(aTime, latitude, longitude));

	const regen::Mat4f zenith = regen::Mat4f::rotationMatrix(0, 0, p - a);
	const regen::Mat4f R(libLat * libLon * zenith);
	return R;
}

float Astronomy::earthShineIntensity(const t_aTime &aTime, float latitude, float longitude) const {
	auto m = moonPosition(aTime, latitude, longitude, false);
	auto s = sunPosition(aTime, latitude, longitude, false);

	// ("Multiple Light Scattering" - 1980 - Van de Hulst) and
	// ("A Physically-Based Night Sky Model" - 2001 - Wann Jensen et al.) -> the 0.19 is the earth full intensity
	const float ep = acos(s.dot(-m));
	const float ep2 = ep * ep;
	const float ep3 = ep * ep2;
	const float Eem = -0.0061f * ep3 + 0.0289f * ep2 - 0.0105f * sin(ep);

	return Eem;
}

regen::Mat4f Astronomy::equToHorTransform(const t_aTime &aTime, float latitude, float longitude) const {
	auto s = static_cast<float>(siderealTime(aTime));
	return regen::Mat4f::scaleMatrix(regen::Vec3f(-1, 1, 1))
		   * regen::Mat4f::rotationMatrix(_rad(latitude) - _PI_2, 0, 0)
		   * regen::Mat4f::rotationMatrix(0, 0, -_rad(s + longitude));
}
