#ifndef REGEN_TIME_H
#define REGEN_TIME_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include "regen/shader/shader-input.h"

namespace regen {
	/**
	 * \brief World time structure.
	 *
	 * This structure holds the world time information,
	 * including the current time and a shader input
	 * that can be used in shaders.
	 */
	struct WorldTime {
		boost::posix_time::ptime p_time;
		ref_ptr<ShaderInput1f> in;
		double scale = 1.0;
	};

	/**
	 * \brief System time structure.
	 *
	 * This structure holds the system time information,
	 * including the current time and a shader input
	 * that can be used in shaders.
	 */
	struct SystemTime {
		boost::posix_time::ptime p_time;
		ref_ptr<ShaderInput1f> in;
	};
}

#endif //REGEN_TIME_H
