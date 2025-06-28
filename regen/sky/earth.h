// Code partly based on the original code from osgHimmel:
//   Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
//   Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany

#ifndef REGEN_EARTH_H_
#define REGEN_EARTH_H_

#include "regen/external/osghimmel/julianday.h"

namespace regen {
	class Earth {
	public:
		static float orbitEccentricity();

		static float apparentAngularSunDiameter(const osgHimmel::t_julianDay &t);

		static float apparentAngularMoonDiameter(const osgHimmel::t_julianDay &t);

		static float longitudeNutation(const osgHimmel::t_julianDay &t);

		static float obliquityNutation(const osgHimmel::t_julianDay &t);

		static float meanObliquity(const osgHimmel::t_julianDay &t);

		static float trueObliquity(const osgHimmel::t_julianDay &t);

		static float viewDistanceWithinAtmosphere(float y);

		static float atmosphericRefraction(float altitude);

		static float meanRadius();

		static float atmosphereThickness();

		static float atmosphereThicknessNonUniform();

		static float apparentMagnitudeLimit();
	};

} // namespace regen

#endif // REGEN_EARTH_H_
