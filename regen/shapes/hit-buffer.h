#ifndef REGEN_HIT_BUFFER_H
#define REGEN_HIT_BUFFER_H

#include <regen/memory/aligned-array.h>

namespace regen {
	/**
	 * @brief Buffer for storing hit shape indices.
	 */
	struct HitBuffer {
		// Aligned array of shape indices
		AlignedArray<uint32_t> data;
		// Current count of hits
		uint32_t count = 0;
		void resize(uint32_t newSize) { data.resize(newSize); }
		void reset() { count = 0; }
		void push(uint32_t v) { data[count++] = v; }
	};
}

#endif //REGEN_HIT_BUFFER_H
