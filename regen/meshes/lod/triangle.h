#ifndef REGEN_TRIANGLE_H
#define REGEN_TRIANGLE_H

#include <cstdint>

namespace regen {
	/**
	 * \brief A triangle structure.
	 *
	 * This structure represents a triangle in a mesh. It contains three vertex indices
	 * and a flag to indicate if the triangle is still active (e.g. not collapsed).
	 */
	struct Triangle {
		// Indices into the original vertex array
		uint32_t v0, v1, v2;
		// A flag to indicate if the triangle is still active, e.g. not collapsed
		bool active = true;

		// A flag to indicate if the triangle is degenerate (i.e. has zero area)
		bool isDegenerate() const {
			return v0 == v1 || v1 == v2 || v2 == v0;
		}

		Triangle(uint32_t _v0, uint32_t _v1, uint32_t _v2)
				: v0(_v0), v1(_v1), v2(_v2) {}
	};
}

#endif //REGEN_TRIANGLE_H
