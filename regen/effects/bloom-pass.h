#ifndef REGEN_BLOOM_PASS_H
#define REGEN_BLOOM_PASS_H

#include <regen/states/fullscreen-pass.h>
#include "regen/textures/fbo-state.h"
#include "bloom-texture.h"

namespace regen {
	/**
	 * \brief Bloom effect pass.
	 * This pass updates a bloom texture. It must be composed with the input texture
	 * to create the final image.
	 */
	class BloomPass : public StateNode {
	public:
		BloomPass(
				const ref_ptr<Texture> &inputTexture,
				const ref_ptr<BloomTexture> &bloomTexture);

		static ref_ptr<BloomPass> load(LoadingContext &ctx, scene::SceneInputNode &input);

		void createShader(const StateConfig &cfg);

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Texture> inputTexture_;
		ref_ptr<BloomTexture> bloomTexture_;
		ref_ptr<Mesh> fullscreenMesh_d_;
		ref_ptr<Mesh> fullscreenMesh_u_;
		uint32_t bloomWidth_ = 0;
		uint32_t bloomHeight_ = 0;

		ref_ptr<FBO> fbo_;

		ref_ptr<ShaderInput2f> inverseInputSize_;
		ref_ptr<ShaderInput2f> inverseViewport_;

		ref_ptr<State> upsampleState_;
		ref_ptr<ShaderState> upsampleShader_;
		int inverseViewportLocUS_;
		int inputTextureLocUS_;

		ref_ptr<State> downsampleState_;
		ref_ptr<ShaderState> downsampleShader_;
		int inverseViewportLocDS_;
		int inverseInputSizeLocDS_;
		int inputTextureLocDS_;

		void downsample(RenderState *rs);

		void upsample(RenderState *rs);
	};
}

#endif //REGEN_BLOOM_PASS_H
