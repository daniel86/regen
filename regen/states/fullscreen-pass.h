#ifndef REGEN_FULLSCREEN_PASS_H_
#define REGEN_FULLSCREEN_PASS_H_

#include <regen/states/state.h>
#include <regen/states/state-node.h>
#include <regen/states/state-configurer.h>
#include "regen/glsl/shader-state.h"
#include <regen/meshes/primitives/rectangle.h>

namespace regen {
	/**
	 * \brief State that updates each texel of the active render target.
	 */
	class FullscreenPass : public State, public HasShader {
	public:
		/**
		 * @param shaderKey the key will be imported when createShader is called.
		 */
		explicit FullscreenPass(const std::string &shaderKey) : State(), HasShader(shaderKey) {
			fullscreenMesh_ = Rectangle::getUnitQuad();
			joinStates(shaderState_);
			joinStates(fullscreenMesh_);
		}

		void setUseIndirectDraw(bool v) { useIndirectDraw_ = v; }

		/**
		 * @param cfg the shader configuration.
		 */
		void createShader(const StateConfig &cfg) override {
			// replicate each mesh LOD numLayer times for indirect multi-layer rendering
			if (useIndirectDraw_) {
				const uint32_t numRenderLayer = cfg.numRenderLayer();
				if (numRenderLayer > 1) {
					REGEN_INFO("Using indirect multi-layer fullscreen-pass with " << numRenderLayer << " layers.");
					fullscreenMesh_->createIndirectDrawBuffer(numRenderLayer);
				}
			}
			shaderState_->createShader(cfg, shaderKey_);
			fullscreenMesh_->updateVAO(cfg, shaderState_->shader());
		}

		static ref_ptr<FullscreenPass> load(LoadingContext &ctx, scene::SceneInputNode &input) {
			if (!input.hasAttribute("shader")) {
				REGEN_WARN("Missing shader attribute for " << input.getDescription() << ".");
				return {};
			}
			ref_ptr<FullscreenPass> fs = ref_ptr<FullscreenPass>::alloc(input.getValue("shader"));
			StateConfigurer shaderConfigurer;
			shaderConfigurer.addNode(ctx.parent().get());
			shaderConfigurer.addState(fs.get());
			fs->createShader(shaderConfigurer.cfg());
			return fs;
		}

		const ref_ptr<Mesh>& fullscreenMesh() const {
			return fullscreenMesh_;
		}

	protected:
		ref_ptr<Mesh> fullscreenMesh_;
		bool useIndirectDraw_ = true;
	};
} // namespace
#endif /* REGEN_FULLSCREEN_PASS_H_ */
