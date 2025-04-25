#ifndef REGEN_CONVERSION_H
#define REGEN_CONVERSION_H

#include <cstdint>

namespace regen::conversion {
	/**
	 * Convert a float to a uint32_t.
	 * @param value The float value to convert.
	 * @return The converted uint32_t value.
	 */
	uint32_t floatToUint(float value);

	/**
	 * Convert a uint32_t to a float.
	 * @param value The uint32_t value to convert.
	 * @return The converted float value.
	 */
	float uintToFloat(uint32_t value);
}

#endif //REGEN_CONVERSION_H
