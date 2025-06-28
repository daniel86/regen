// Code partly based on the original code from osgHimmel:
//   Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
//   Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany

#include "earth.h"
#include "moon.h"
#include "sun.h"

using namespace regen;

float Earth::orbitEccentricity() {
	// The linear eccentricity of the earth orbit is about 2.5 * 10^6 km.
	// Compared to the avg. distance of 149.6 * 10^6 km this is not much.
	const float E = 0.01671022;
	return _revd(E);
}

float Earth::apparentAngularSunDiameter(const osgHimmel::t_julianDay &t) {
	return _adiameter(Sun::distance(t), Sun::meanRadius());
}

float Earth::apparentAngularMoonDiameter(const osgHimmel::t_julianDay &t) {
	return _adiameter(Moon::distance(t), Moon::meanRadius());
}

float Earth::longitudeNutation(const osgHimmel::t_julianDay &t) {
	const float sM = _rad(Sun::meanAnomaly(t));
	const float mM = _rad(Moon::meanAnomaly(t));
	const float O = _rad(Moon::meanOrbitLongitude(t));
	// (AA.21 p132)
	const float r =
			-_decimal(0, 0, 17.20f) * sinf(O)
			- _decimal(0, 0, 1.32f) * sinf(2.0f * sM)
			- _decimal(0, 0, 0.23f) * sinf(2.0f * mM)
			+ _decimal(0, 0, 0.21f) * sinf(2.0f * O);

	return r;
}

float Earth::obliquityNutation(const osgHimmel::t_julianDay &t) {
	const float O = _rad(Moon::meanOrbitLongitude(t));
	const float Ls = _rad(Sun::meanAnomaly(t));
	const float Lm = _rad(Moon::meanAnomaly(t));
	// (AA.21 p132)
	const float e =
			+_decimal(0, 0, 9.20f) * cosf(O)
			+ _decimal(0, 0, 0.57f) * cosf(2.0f * Ls)
			+ _decimal(0, 0, 0.10f) * cosf(2.0f * Lm)
			- _decimal(0, 0, 0.09f) * cosf(2.0f * O);

	return e;
}

float Earth::trueObliquity(const osgHimmel::t_julianDay &t) {
	return meanObliquity(t) + obliquityNutation(t); // e
}

float Earth::meanObliquity(const osgHimmel::t_julianDay &t) {
	// Inclination of the Earth's axis of rotation. (AA.21.3)
	// By J. Laskar, "Astronomy and Astrophysics" 1986
	const osgHimmel::t_julianDay U = osgHimmel::jCenturiesSinceSE(t) * 0.01;
	const osgHimmel::t_longf e0 = 0.0
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
	osgHimmel::t_longf R = 1.02 /
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
