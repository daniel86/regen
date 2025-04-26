#ifndef REGEN_CONVERSION_H
#define REGEN_CONVERSION_H

#include <cstdint>

namespace regen::conversion {
	/**
	 * Convert a float to a uint32_t.
	 */
	uint32_t floatBitsToUint(float value);

	/**
	 * Convert a float to a int32_t.
	 */
	int32_t floatBitsToInt(float value);

	/**
	 * Convert a uint32_t to a float.
	 */
	float uintToFloat(uint32_t value);

	/**
	 * Convert a int32_t to a float.
	 */
	float intBitsToFloat(int32_t value);
}

#endif //REGEN_CONVERSION_H
