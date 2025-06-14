#ifndef REGEN_LOD_LEVEL_H
#define REGEN_LOD_LEVEL_H

#include <vector>
#include "lod-attribute.h"

namespace regen {
	/**
	 * \brief A level of detail (LOD) level.
	 *
	 * This structure represents a level of detail (LOD) level in a mesh.
	 * It contains the vertex positions and attributes for that LOD level.
	 */
	struct LODLevel {
		~LODLevel() {
			for (auto &attr: attributes) {
				delete attr;
			}
		}

		LODLevel() = default;

		LODLevel(const LODLevel &other) = delete;

		LODLevel(LODLevel &&other) noexcept :
				pos(std::move(other.pos)),
				attributes(std::move(other.attributes)),
				offset(other.offset) {
			other.offset = 0;
			other.attributes.clear();
		}

		std::vector<Vec3f> pos;
		std::vector<LODAttribute *> attributes;
		// vertex offset of this LOD level
		unsigned int offset = 0;
	};

	/**
	 * \brief Quality of the LOD level.
	 *
	 * This enum represents the quality of the LOD level.
	 * It can be used to determine which LOD level to use based on the distance to the camera.
	 */
	enum class LODQuality {
		LOW,
		MEDIUM,
		HIGH
	};

	std::ostream &operator<<(std::ostream &out, const LODQuality &quality);

	std::istream &operator>>(std::istream &in, LODQuality &quality);
}

#endif //REGEN_LOD_LEVEL_H
