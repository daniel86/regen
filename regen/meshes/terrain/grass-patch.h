#ifndef REGEN_GRASS_PATCH_H_
#define REGEN_GRASS_PATCH_H_

#include <regen/meshes/mask-mesh.h>
#include <regen/states/model-transformation.h>

namespace regen {
	class GrassPatch : public Mesh {
	public:
		GrassPatch(
				const ref_ptr<ModelTransformation> &tf,
				const ref_ptr<Texture2D> &maskTexture,
				uint32_t maskIndex,
				const MaskMesh::Config &patchConfig = MaskMesh::Config());

		/**
		 * Updates vertex data based on given configuration.
		 * @param cfg vertex data configuration.
		 */
		virtual void updateAttributes();

		void updateTransforms();

		/**
		 * Load a grass mesh from a property tree.
		 * @param ctx the loading context.
		 * @param input the input node.
		 * @return the loaded ground mesh.
		 */
		static ref_ptr<GrassPatch> load(
				LoadingContext &ctx,
				scene::SceneInputNode &input,
				const Rectangle::Config &quadCfg);

	protected:
		ref_ptr<MaskMesh> maskMesh_;
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput3f> basePos_;
		ref_ptr<ShaderInput1ui> indices_;

		void generateLODLevel(uint32_t lodLevel);
	};

} // namespace

#endif /* REGEN_GRASS_PATCH_H_ */
