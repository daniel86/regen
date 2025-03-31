#ifndef REGEN_COMPUTE_PASS_H_
#define REGEN_COMPUTE_PASS_H_

#include <regen/states/state.h>
#include <regen/states/state-node.h>
#include <regen/states/state-configurer.h>
#include <regen/states/shader-state.h>
#include <regen/states/compute-state.h>

namespace regen {
	/**
	 * \brief State that
	 */
	class ComputePass : public State, public HasShader {
	public:
		/**
		 * @param shaderKey the key will be imported when createShader is called.
		 */
		explicit ComputePass(const std::string &shaderKey) : State(), HasShader(shaderKey) {
			computeState_ = ref_ptr<ComputeState>::alloc();
			joinStates(shaderState_);
			joinStates(computeState_);
		}

		/**
		 * @return the compute state.
		 */
		auto &computeState() const { return computeState_; }

		/**
		 * @param cfg the shader configuration.
		 */
		void createShader(const StateConfig &cfg) override {
			shaderState_->createShader(cfg, shaderKey_);
		}

		static ref_ptr<ComputePass> load(LoadingContext &ctx, scene::SceneInputNode &input) {
			if (!input.hasAttribute("shader")) {
				REGEN_WARN("Missing shader attribute for " << input.getDescription() << ".");
				return {};
			}
			ref_ptr<ComputePass> cs = ref_ptr<ComputePass>::alloc(input.getValue("shader"));
			StateConfigurer shaderConfigurer;
			shaderConfigurer.addNode(ctx.parent().get());
			shaderConfigurer.addState(cs.get());
			cs->createShader(shaderConfigurer.cfg());
			return cs;
		}

	protected:
		ref_ptr<ComputeState> computeState_;
	};
} // namespace
#endif /* FULLSCREEN_PASS_H_ */
