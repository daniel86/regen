// Code partly based on the original code from osgHimmel:
//   Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
//   Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany

#ifndef REGEN_SUN_H_
#define REGEN_SUN_H_

#include "regen/external/osghimmel/julianday.h"
#include "regen/external/osghimmel/coords.h"

namespace regen {
	class Sun {
	public:
		static float meanAnomaly(const osgHimmel::t_julianDay &t);

		static float meanLongitude(const osgHimmel::t_julianDay &t);

		static osgHimmel::t_equf apparentPosition(const osgHimmel::t_julianDay &t);

		static osgHimmel::t_horf horizontalPosition(const osgHimmel::t_aTime &aTime, float latitude, float longitude);

		static float distance(const osgHimmel::t_julianDay &t);

		static float meanRadius();
	};

} // namespace regen

#endif // REGEN_SUN_H_
