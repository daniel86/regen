#include "conversion.h"

namespace regen::conversion {
	uint32_t floatBitsToUint(float value) {
		union {
			float floatValue;
		 uint32_t uintValue;
		} converter;
		converter.floatValue = value;
		return converter.uintValue;
	}

	int32_t floatBitsToInt(float value) {
		union {
			float floatValue;
			int32_t intValue;
		} converter;
		converter.floatValue = value;
		return converter.intValue;
	}

	float uintToFloat(uint32_t value) {
		union {
			uint32_t uintValue;
			float floatValue;
		} converter;
		converter.uintValue = value;
		return converter.floatValue;
	}

	float intBitsToFloat(int32_t value) {
		union {
			int32_t intValue;
			float floatValue;
		} converter;
		converter.intValue = value;
		return converter.floatValue;
	}
}
