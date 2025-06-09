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

#include "earth.h"

#include "sun.h"
#include "moon.h"
#include "mathmacros.h"

using namespace osgHimmel;

float Earth::orbitEccentricity() {
	// The linear eccentricity of the earth orbit is about 2.5 * 10^6 km.
	// Compared to the avg. distance of 149.6 * 10^6 km this is not much.
	// http://www.greier-greiner.at/hc/ekliptik.htm
	// P. Bretagnon, "Théorie du mouvement de l'ensamble des planètes. Solution VSOP82", 1982
	// http://nssdc.gsfc.nasa.gov/planetary/factsheet/earthfact.html
	const float E = 0.01671022;
	return _revd(E);
}

float Earth::apparentAngularSunDiameter(const t_julianDay &t) {
	return _adiameter(Sun::distance(t), Sun::meanRadius());
}

float Earth::apparentAngularMoonDiameter(const t_julianDay &t) {
	return _adiameter(Moon::distance(t), Moon::meanRadius());
}

float Earth::longitudeNutation(const t_julianDay &t) {
	const float sM = _rad(Sun::meanAnomaly(t));
	const float mM = _rad(Moon::meanAnomaly(t));
	const float O = _rad(Moon::meanOrbitLongitude(t));

	// (AA.21 p132)
	const float r =
			- _decimal(0, 0, 17.20) * sin(O)
			- _decimal(0, 0, 1.32) * sin(2.0 * sM)
			- _decimal(0, 0, 0.23) * sin(2.0 * mM)
			+ _decimal(0, 0, 0.21) * sin(2.0 * O);

	return r;
}

float Earth::obliquityNutation(const t_julianDay &t) {
	const float O = _rad(Moon::meanOrbitLongitude(t));
	const float Ls = _rad(Sun::meanAnomaly(t));
	const float Lm = _rad(Moon::meanAnomaly(t));

	// (AA.21 p132)
	const float e =
			+ _decimal(0, 0, 9.20) * cos(O)
			+ _decimal(0, 0, 0.57) * cos(2.0 * Ls)
			+ _decimal(0, 0, 0.10) * cos(2.0 * Lm)
			- _decimal(0, 0, 0.09) * cos(2.0 * O);

	return e;
}

float Earth::trueObliquity(const t_julianDay &t) {
	return meanObliquity(t) + obliquityNutation(t); // e
}

float Earth::meanObliquity(const t_julianDay &t) {
	// Inclination of the Earth's axis of rotation. (AA.21.3)
	// By J. Laskar, "Astronomy and Astrophysics" 1986
	const t_julianDay U = jCenturiesSinceSE(t) * 0.01;

	assert(_abs(U) < 1.0);

	const t_longf e0 = 0.0
		+ U * (-4680.93
		+ U * (-1.55
		+ U * (+1999.25
		+ U * (-51.38
		+ U * (-249.67
		+ U * (-39.05
		+ U * (+7.12
		+ U * (+27.87
		+ U * (+5.79
		+ U * (+2.45))))))))));

	return _decimal(23, 26, 21.448) + _decimal(0, 0, e0);
}

float Earth::viewDistanceWithinAtmosphere(float y) {
	// This is not refraction corrected.
	const float t = atmosphereThickness();
	//const float r = meanRadius();
	// This works only for mean radius of earth.
	return t * 1116.0 / ((y + 0.004) * 1.1116);
}

float Earth::atmosphericRefraction(float altitude) {
	// Effect of refraction for true altitudes (AA.15.4).
	// G.G. Bennet, "The Calculation of the Astronomical Refraction in marine Navigation", 1982
	// and Porsteinn Saemundsson, "Sky and Telescope" 1982
	t_longf R = 1.02 /
				tan(_rad(altitude + 10.3 / (altitude + 5.11))) + 0.0019279;
	return _decimal(0, R, 0); // (since R is in minutes)
}

float Earth::meanRadius() {
	// http://nssdc.gsfc.nasa.gov/planetary/factsheet/earthfact.html
	return 6371.0f; // in kilometers
}

float Earth::atmosphereThickness() {
	// Thickness of atmosphere if the density were uniform.
	// 8000 ("Precomputed Atmospheric Scattering" - 2008 - Bruneton, Neyret)
	// 7994 ("Display of the earth taking into account atmospheric scattering" - 1993 - Nishita et al.)
	return 7.994f;
}

float Earth::atmosphereThicknessNonUniform() {
	// Thickness of atmosphere.
	return 85.0f; // ~
}

float Earth::apparentMagnitudeLimit() {
	// http://www.astronomynotes.com/starprop/s4.htm
	return 6.5f;
}
