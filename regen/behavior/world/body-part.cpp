#include "body-part.h"
#include "regen/utility/logging.h"

using namespace regen;

std::ostream &regen::operator<<(std::ostream &out, const BodyPart &v) {
	switch (v) {
		case BodyPart::HEAD:
			return out << "HEAD";
		case BodyPart::NECK:
			return out << "NECK";
		case BodyPart::TORSO:
			return out << "TORSO";
		case BodyPart::ARM:
			return out << "ARM";
		case BodyPart::LEG:
			return out << "LEG";
		case BodyPart::LAST:
			return out << "NO_BODY_PART";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BodyPart &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "HEAD") v = BodyPart::HEAD;
	else if (val == "NECK") v = BodyPart::NECK;
	else if (val == "TORSO") v = BodyPart::TORSO;
	else if (val == "ARM") v = BodyPart::ARM;
	else if (val == "LEG") v = BodyPart::LEG;
	else {
		REGEN_WARN("Unknown BodyPart value: " << val);
		v = BodyPart::HEAD;
	}
	return in;
}
