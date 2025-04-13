#ifndef REGEN_MASK_MESH_H
#define REGEN_MASK_MESH_H

#include <regen/meshes/mesh-state.h>
#include <regen/shapes/bounds.h>
#include "lod/tessellation.h"
#include "regen/meshes/primitives/rectangle.h"
#include <regen/textures/texture-2d.h>

namespace regen {
	/**
	 * \brief A series of patches in xz plane covering a mask defined by a texture.
	 */
	class MaskMesh : public Rectangle {
	public:
		/**
		 * Vertex data configuration.
		 */
		struct Config {
			Rectangle::Config quad;
			/** scaling of the whole mesh */
			Vec2f meshSize;
			/** y offset of all vertices */
			float height;
			/** optional height map */
			ref_ptr<Texture2D> heightMap;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit MaskMesh(const ref_ptr<Texture2D> &maskTexture, const Config &cfg = Config());

		/**
		 * @param other Another Rectangle.
		 */
		explicit MaskMesh(const ref_ptr<MaskMesh> &other);

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateMask(const Config &cfg);

	protected:
		ref_ptr<Texture2D> maskTexture_;
		ref_ptr<ShaderInput3f> modelOffset_;
		Vec2f meshSize_;
	};
} // namespace

#endif /* REGEN_MASK_MESH_H */
