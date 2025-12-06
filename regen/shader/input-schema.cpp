#include <utility>
#include <map>
#include <boost/algorithm/string/case_conv.hpp>
#include "input-schema.h"

using namespace regen;

InputSchema::InputSchema(Semantics semantics)
	: semantics_(semantics) {
}

std::optional<std::pair<float, float>> InputSchema::limits(int index) const {
	auto it = limits_.find(index);
	if (it == limits_.end()) {
		return std::nullopt;
	}
	return it->second;
}

void InputSchema::setLimits(int index, float min, float max) {
	limits_[index] = std::make_pair(min, max);
}

const InputSchema *InputSchema::getDefault(Semantics semantics) {
	switch (semantics) {
		case SCALAR:
			return scalar(0, 1);
		case COLOR:
			return color();
		case ALPHA:
			return alpha();
		case POSITION:
			return position();
		case DIRECTION:
			return direction();
		case SCALE:
			return scale();
		case TRANSFORM:
			return transform();
		case TEXTURE:
			return texture();
		default:
			return unknown();
	}
}

const InputSchema *InputSchema::unknown() {
	static const InputSchema unknown_(InputSchema::UNKNOWN);
	return &unknown_;
}

const InputSchema *InputSchema::color() {
	static const InputSchema *color_ = []() {
		auto *schema = new InputSchema(InputSchema::COLOR);
		schema->setLimits(0, 0, 1);
		schema->setLimits(1, 0, 1);
		schema->setLimits(2, 0, 1);
		schema->setLimits(3, 0, 1);
		return schema;
	}();
	return color_;
}

const InputSchema *InputSchema::alpha() {
	static const InputSchema *alpha_ = []() {
		auto *schema = new InputSchema(InputSchema::ALPHA);
		schema->setLimits(0, 0, 1);
		return schema;
	}();
	return alpha_;
}

const InputSchema *InputSchema::position() {
	static const InputSchema position_(InputSchema::POSITION);
	return &position_;
}

const InputSchema *InputSchema::direction() {
	static const InputSchema direction_(InputSchema::DIRECTION);
	return &direction_;
}

const InputSchema *InputSchema::scale() {
	static const InputSchema scale_(InputSchema::SCALE);
	return &scale_;
}

const InputSchema *InputSchema::transform() {
	static const InputSchema transform_(InputSchema::TRANSFORM);
	return &transform_;
}

const InputSchema *InputSchema::texture() {
	static const InputSchema texture_(InputSchema::TEXTURE);
	return &texture_;
}

const InputSchema *InputSchema::scalar(float min, float max) {
	static std::map<std::pair<float, float>, InputSchema> scalarSchemas;
	auto key = std::make_pair(min, max);
	auto it = scalarSchemas.find(key);
	if (it == scalarSchemas.end()) {
		auto inserted = scalarSchemas.emplace(key, InputSchema::SCALAR);
		it = inserted.first;
		it->second.setLimits(0, min, max);
	}
	return &it->second;
}

InputSchema *InputSchema::alloc(Semantics semantics) {
	static std::vector<InputSchema> schemas;
	auto &newSchema = schemas.emplace_back(semantics);
	return &newSchema;
}

std::ostream &regen::operator<<(std::ostream &out, const InputSchema::Semantics &semantics) {
	switch (semantics) {
		case InputSchema::UNKNOWN:
			out << "UNKNOWN";
			break;
		case InputSchema::VECTOR:
			out << "VECTOR";
			break;
		case InputSchema::SCALAR:
			out << "SCALAR";
			break;
		case InputSchema::COLOR:
			out << "COLOR";
			break;
		case InputSchema::ALPHA:
			out << "ALPHA";
			break;
		case InputSchema::POSITION:
			out << "POSITION";
			break;
		case InputSchema::DIRECTION:
			out << "DIRECTION";
			break;
		case InputSchema::SCALE:
			out << "SCALE";
			break;
		case InputSchema::TRANSFORM:
			out << "TRANSFORM";
			break;
		case InputSchema::TEXTURE:
			out << "TEXTURE";
			break;
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, InputSchema::Semantics &semantics) {
	std::string s;
	in >> s;
	boost::to_upper(s);
	if (s == "UNKNOWN") {
		semantics = InputSchema::UNKNOWN;
	} else if (s == "VECTOR") {
		semantics = InputSchema::VECTOR;
	} else if (s == "SCALAR") {
		semantics = InputSchema::SCALAR;
	} else if (s == "COLOR") {
		semantics = InputSchema::COLOR;
	} else if (s == "ALPHA") {
		semantics = InputSchema::ALPHA;
	} else if (s == "POSITION") {
		semantics = InputSchema::POSITION;
	} else if (s == "DIRECTION") {
		semantics = InputSchema::DIRECTION;
	} else if (s == "SCALE") {
		semantics = InputSchema::SCALE;
	} else if (s == "TRANSFORM") {
		semantics = InputSchema::TRANSFORM;
	} else if (s == "TEXTURE") {
		semantics = InputSchema::TEXTURE;
	}
	return in;
}
