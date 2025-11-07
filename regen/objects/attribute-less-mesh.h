#ifndef REGEN_ATTRIBUTE_LESS_MESH_STATE_H_
#define REGEN_ATTRIBUTE_LESS_MESH_STATE_H_

#include <regen/objects/mesh.h>

namespace regen {
	/**
	 * \brief Mesh that can be used when no vertex shader input
	 * is required.
	 *
	 * This effectively means that you have to generate
	 * geometry that will be rasterized.
	 */
	class AttributeLessMesh : public Mesh {
	public:
		/**
		 * @param numVertices number of vertices used.
		 */
		explicit AttributeLessMesh(GLuint numVertices);
	};
} // namespace

#endif /* REGEN_MESH_STATE_H_ */
