#include "lod-level.h"

namespace regen {
	std::ostream &operator<<(std::ostream &out, const LODQuality &quality) {
		switch (quality) {
			case LODQuality::LOW: return out << "low";
			case LODQuality::MEDIUM: return out << "medium";
			case LODQuality::HIGH: return out << "high";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, LODQuality &quality) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "low") quality = LODQuality::LOW;
		else if (val == "medium") quality = LODQuality::MEDIUM;
		else if (val == "high") quality = LODQuality::HIGH;
		else {
			REGEN_WARN("Unknown LOD quality '" << val << "'. Using default HIGH.");
			quality = LODQuality::HIGH;
		}
		return in;
	}
}
