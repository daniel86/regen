#ifndef REGEN_TIME_H
#define REGEN_TIME_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include "regen/gl-types/shader-input.h"

namespace regen {
	struct WorldTime {
		boost::posix_time::ptime p_time;
		ref_ptr<ShaderInput1f> in;
		double scale = 1.0;
	};
}

#endif //REGEN_TIME_H
