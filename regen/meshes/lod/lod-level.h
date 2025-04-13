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
}

#endif //REGEN_LOD_LEVEL_H
