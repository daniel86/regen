#include "motion-type.h"

#include "regen/behavior/world/body-part.h"
#include "regen/utility/logging.h"

using namespace regen;

bool regen::isInterruptibleMotion(MotionType type) {
	return (type != MotionType::ATTACK && type != MotionType::BLOCK);
}

const float* regen::motionBodyPartWeights(MotionType type) {
	static constexpr auto numMotions = static_cast<uint32_t>(MotionType::MOTION_LAST);
	static constexpr auto numBodyParts = static_cast<uint32_t>(BodyPart::LAST);
	static constexpr std::array<std::array<float, numBodyParts + 1>, numMotions> bodyPartWeights = {{
		// BASE, HEAD, NECK, TORSO, ARM, LEG
		[static_cast<size_t>(MotionType::RUN)]        = { 1.0f, 0.2f, 0.2f, 0.25f, 0.3f, 1.0f },
		[static_cast<size_t>(MotionType::WALK)]       = { 0.8f, 0.2f, 0.2f, 0.3f, 0.25f, 0.9f },
		[static_cast<size_t>(MotionType::JUMP)]       = { 1.0f, 0.3f, 0.3f, 0.7f, 0.6f, 1.0f },
		[static_cast<size_t>(MotionType::SWIM)]       = { 0.8f, 0.4f, 0.4f, 0.6f, 0.7f, 0.8f },
		[static_cast<size_t>(MotionType::IDLE)]       = { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f },
		[static_cast<size_t>(MotionType::SIT)]        = { 0.2f, 0.3f, 0.3f, 0.4f, 0.2f, 1.0f },
		[static_cast<size_t>(MotionType::AGREE)]      = { 0.1f, 1.0f, 0.6f, 0.3f, 0.3f, 0.1f },
		[static_cast<size_t>(MotionType::DISAGREE)]   = { 0.1f, 1.0f, 0.6f, 0.3f, 0.3f, 0.1f },
		[static_cast<size_t>(MotionType::VOCALIZE)]   = { 0.1f, 1.0f, 0.5f, 0.3f, 0.2f, 0.1f },
		[static_cast<size_t>(MotionType::ATTACK)]     = { 0.5f, 0.2f, 0.3f, 0.6f, 1.0f, 0.3f },
		[static_cast<size_t>(MotionType::BLOCK)]      = { 0.5f, 0.2f, 0.3f, 0.6f, 1.0f, 0.3f },
		[static_cast<size_t>(MotionType::CROUCH)]     = { 0.2f, 0.3f, 0.3f, 0.4f, 0.3f, 1.0f },
		[static_cast<size_t>(MotionType::INSPECT)]    = { 0.2f, 1.0f, 0.8f, 0.4f, 0.3f, 0.2f },
		[static_cast<size_t>(MotionType::INTIMIDATE)] = { 0.7f, 0.6f, 0.5f, 0.8f, 1.0f, 0.5f },
		[static_cast<size_t>(MotionType::STRETCH)]    = { 0.5f, 0.4f, 0.4f, 0.6f, 0.7f, 0.7f },
		[static_cast<size_t>(MotionType::GROOMING)]   = { 0.6f, 0.8f, 0.8f, 0.6f, 0.7f, 0.3f },
		[static_cast<size_t>(MotionType::PRAY)]       = { 0.2f, 0.5f, 0.4f, 0.5f, 0.6f, 0.6f },
		[static_cast<size_t>(MotionType::SLEEP)]      = { 0.2f, 0.3f, 0.3f, 0.5f, 0.4f, 0.8f },
		[static_cast<size_t>(MotionType::DIE)]        = { 1.0f, 0.6f, 0.6f, 1.0f, 0.7f, 0.8f },
		[static_cast<size_t>(MotionType::REVIVE)]     = { 0.8f, 0.4f, 0.4f, 0.8f, 0.7f, 1.0f }
	}};
	return bodyPartWeights[static_cast<uint32_t>(type)].data();
}

std::ostream &regen::operator<<(std::ostream &out, const MotionType &v) {
	switch (v) {
		case MotionType::IDLE:
			return out << "IDLE";
		case MotionType::WALK:
			return out << "WALK";
		case MotionType::RUN:
			return out << "RUN";
		case MotionType::SWIM:
			return out << "SWIM";
		case MotionType::JUMP:
			return out << "JUMP";
		case MotionType::SIT:
			return out << "SIT";
		case MotionType::AGREE:
			return out << "AGREE";
		case MotionType::DISAGREE:
			return out << "DISAGREE";
		case MotionType::VOCALIZE:
			return out << "VOCALIZE";
		case MotionType::ATTACK:
			return out << "ATTACK";
		case MotionType::BLOCK:
			return out << "BLOCK";
		case MotionType::CROUCH:
			return out << "CROUCH";
		case MotionType::INSPECT:
			return out << "INSPECT";
		case MotionType::INTIMIDATE:
			return out << "INTIMIDATE";
		case MotionType::STRETCH:
			return out << "STRETCH";
		case MotionType::GROOMING:
			return out << "GROOMING";
		case MotionType::PRAY:
			return out << "PRAY";
		case MotionType::SLEEP:
			return out << "SLEEP";
		case MotionType::DIE:
			return out << "DIE";
		case MotionType::REVIVE:
			return out << "REVIVE";
		case MotionType::MOTION_LAST:
			return out << "MOTION_LAST";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, MotionType &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "IDLE") v = MotionType::IDLE;
	else if (val == "WALK") v = MotionType::WALK;
	else if (val == "RUN") v = MotionType::RUN;
	else if (val == "JUMP") v = MotionType::JUMP;
	else if (val == "SIT") v = MotionType::SIT;
	else if (val == "AGREE" || val == "YES" || val == "NOD") v = MotionType::AGREE;
	else if (val == "DISAGREE" || val == "NO" || val == "SHAKE") v = MotionType::DISAGREE;
	else if (val == "ATTACK" || val == "FIGHT") v = MotionType::ATTACK;
	else if (val == "BLOCK" || val == "SHIELD") v = MotionType::BLOCK;
	else if (val == "CROUCH" || val == "SNEAK") v = MotionType::CROUCH;
	else if (val == "SWIM") v = MotionType::SWIM;
	else if (val == "VOCALIZE" || val == "TALK" || val == "BARK") v = MotionType::VOCALIZE;
	else if (val == "INSPECT" || val == "LOOK_AROUND" || val == "SNIFF") v = MotionType::INSPECT;
	else if (val == "INTIMIDATE" || val == "THREATEN" || val == "ROAR") v = MotionType::INTIMIDATE;
	else if (val == "STRETCH") v = MotionType::STRETCH;
	else if (val == "GROOMING" || val == "GROOM") v = MotionType::GROOMING;
	else if (val == "PRAY" || val == "PRAYING" || val == "WORSHIP") v = MotionType::PRAY;
	else if (val == "SLEEP" || val == "SLEEPING") v = MotionType::SLEEP;
	else if (val == "DIE" || val == "FALL") v = MotionType::DIE;
	else if (val == "REVIVE" || val == "STAND_UP" || val == "GET_UP") v = MotionType::REVIVE;
	else if (val == "MOTION_LAST") v = MotionType::MOTION_LAST;
	else {
		REGEN_WARN("Unknown action type '" << val << "'. Using IDLE.");
		v = MotionType::IDLE;
	}
	return in;
}
