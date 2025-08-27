#include <boost/algorithm/string/case_conv.hpp>
#include "regen/camera/sorting.h"
#include "regen/utility/logging.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const SortMode &mode) {
		switch (mode) {
			case FRONT_TO_BACK:
				return out << "front-to-back";
			case BACK_TO_FRONT:
				return out << "back-to-front";
			case NO_SORTING:
				return out << "no-sorting";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, SortMode &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "front-to-back") mode = FRONT_TO_BACK;
		else if (val == "back-to-front") mode = BACK_TO_FRONT;
		else if (val == "no-sorting") mode = NO_SORTING;
		else {
			REGEN_WARN("Unknown Texture Mapping '" << val <<
												   "'. Using default CUSTOM Mapping.");
			mode = BACK_TO_FRONT;
		}
		return in;
	}
}
