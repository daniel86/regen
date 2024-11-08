#ifndef REGEN_BLOOM_PASS_H
#define REGEN_BLOOM_PASS_H

#include <regen/states/fullscreen-pass.h>
#include <regen/states/fbo-state.h>
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

		void createShader(const StateConfig &cfg);

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Texture> inputTexture_;
		ref_ptr<BloomTexture> bloomTexture_;
		ref_ptr<Mesh> fullscreenMesh_;

		ref_ptr<FBO> fbo_;

		ref_ptr<ShaderInput2f> inverseInputSize_;
		ref_ptr<ShaderInput2f> inverseViewport_;

		ref_ptr<State> upsampleState_;
		ref_ptr<ShaderState> upsampleShader_;
		GLint inverseViewportLocUS_;

		ref_ptr<State> downsampleState_;
		ref_ptr<ShaderState> downsampleShader_;
		GLint inverseViewportLocDS_;
		GLint inverseInputSizeLocDS_;

		void downsample(RenderState *rs);
		void upsample(RenderState *rs);
	};
}

#endif //REGEN_BLOOM_PASS_H
