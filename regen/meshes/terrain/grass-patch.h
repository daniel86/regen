#ifndef REGEN_GRASS_PATCH_H_
#define REGEN_GRASS_PATCH_H_

#include <regen/meshes/mask-mesh.h>
#include <regen/states/model-transformation.h>

namespace regen {
	class GrassPatch : public MaskMesh {
	public:
		GrassPatch(
				const ref_ptr<ModelTransformation> &tf,
				const ref_ptr<Texture2D> &maskTexture,
				uint32_t maskIndex,
				const Config &cfg = Config());

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

	};

} // namespace

#endif /* REGEN_GRASS_PATCH_H_ */
