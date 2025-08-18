#ifndef REGEN_MASK_MESH_H
#define REGEN_MASK_MESH_H

#include <regen/meshes/mesh-state.h>
#include <regen/shapes/bounds.h>
#include "lod/tessellation.h"
#include "regen/meshes/primitives/rectangle.h"
#include <regen/textures/texture.h>
#include <regen/textures/texture-state.h>
#include <regen/states/model-transformation.h>

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
		MaskMesh(
				const ref_ptr<ModelTransformation> &tf,
				const ref_ptr<Texture2D> &maskTexture,
				uint32_t maskIndex,
				const Config &cfg = Config());

		/**
		 * @return the model transformation assigned to this ground.
		 */
		const ref_ptr<ModelTransformation> &tf() const { return tf_; }

		/**
		 * @return the mask texture.
		 */
		const ref_ptr<Texture2D> &maskTexture() const { return maskTexture_; }

		/**
		 * @return the mask texture state.
		 */
		const ref_ptr<TextureState> &maskTextureState() const { return maskTextureState_; }

		/**
		 * @return the mask index.
		 */
		uint32_t maskIndex() const { return maskIndex_; }

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		void updateMask();

		/**
		 * Load a mask mesh from a property tree.
		 * @param ctx the loading context.
		 * @param input the input node.
		 * @return the loaded ground mesh.
		 */
		static ref_ptr<MaskMesh> load(LoadingContext &ctx,
				scene::SceneInputNode &input,
				const Rectangle::Config &quadCfg);

	protected:
		Config maskMeshCfg_;
		ref_ptr<ModelTransformation> tf_;
		ref_ptr<Texture2D> maskTexture_;
		ref_ptr<TextureState> maskTextureState_;
		uint32_t maskIndex_;
		Vec2f meshSize_;
	};
} // namespace

#endif /* REGEN_MASK_MESH_H */
