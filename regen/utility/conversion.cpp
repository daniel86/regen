#include "conversion.h"

namespace regen::conversion {
	uint32_t floatToUint(float value) {
		union {
			float floatValue;
		 uint32_t uintValue;
		} converter;
		converter.floatValue = value;
		return converter.uintValue;
	}

	float uintToFloat(uint32_t value) {
		union {
			uint32_t uintValue;
			float floatValue;
		} converter;
		converter.uintValue = value;
		return converter.floatValue;
	}
}
