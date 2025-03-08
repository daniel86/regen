/*
 * fullscreen-pass.h
 *
 *  Created on: 09.03.2013
 *      Author: daniel
 */

#ifndef FULLSCREEN_PASS_H_
#define FULLSCREEN_PASS_H_

#include <regen/states/state.h>
#include <regen/states/state-node.h>
#include <regen/states/state-configurer.h>
#include <regen/states/shader-state.h>
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

		/**
		 * @param cfg the shader configuration.
		 */
		void createShader(const StateConfig &cfg) override {
			shaderState_->createShader(cfg, shaderKey_);
			fullscreenMesh_->updateVAO(RenderState::get(), cfg, shaderState_->shader());
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

	protected:
		ref_ptr<Mesh> fullscreenMesh_;
	};
} // namespace
#endif /* FULLSCREEN_PASS_H_ */
