// Code partly based on the original code from osgHimmel:
//   Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
//   Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany

#include "sun.h"
#include "earth.h"

#include "regen/external/osghimmel/siderealtime.h"

using namespace regen;

float Sun::meanAnomaly(const osgHimmel::t_julianDay &t) {
	const osgHimmel::t_julianDay T(osgHimmel::jCenturiesSinceSE(t));
	const float M = _deg(6.24 + 628.302 * T);
	return _revd(M);
}

float Sun::meanLongitude(const osgHimmel::t_julianDay &t) {
	const osgHimmel::t_julianDay T(osgHimmel::jCenturiesSinceSE(t));
	const float L0 = 280.4665f + T * (+36000.7698f);
	return _revd(L0);
}

osgHimmel::t_equf Sun::apparentPosition(const osgHimmel::t_julianDay &t) {
	const osgHimmel::t_julianDay T(osgHimmel::jCenturiesSinceSE(t));
	const float M = _rad(meanAnomaly(t));

	osgHimmel::t_eclf ecl;
	ecl.longitude = _deg(4.895048 + 628.331951 * T
						 + (0.033417 - 0.000084 * T) * sin(M) + 0.000351 * sin(2 * M));
	ecl.latitude = 0;

	osgHimmel::t_equf equ;
	equ = ecl.toEquatorial(Earth::trueObliquity(t));
	return equ;
}

osgHimmel::t_horf Sun::horizontalPosition(const osgHimmel::t_aTime &aTime, float latitude, float longitude) {
	osgHimmel::t_julianDay t(jd(aTime));
	osgHimmel::t_julianDay s(siderealTime(aTime));
	osgHimmel::t_equf equ = apparentPosition(t);
	return equ.toHorizontal(s, latitude, longitude);
}

float Sun::distance(const osgHimmel::t_julianDay &t) {
	const osgHimmel::t_julianDay T(osgHimmel::jCenturiesSinceSE(t));
	const float M = 6.24f + 628.302f * T;
	const float R = 1.000140f - (0.016708f - 0.000042f * T) * cos(M) - 0.000141f * cos(2 * M); // in AU
	return _kms(R);
}

float Sun::meanRadius() {
	// http://nssdc.gsfc.nasa.gov/planetary/factsheet/sunfact.html
	return 0.696e+6; // in kilometers
}
