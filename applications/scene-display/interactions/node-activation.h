#include <regen/textures/texture-state.h>
#include <regen/av/video-texture.h>
#include <regen/scene/scene-interaction.h>
#include <regen/scene/scene.h>

namespace regen {
	class NodeActivation : public SceneInteraction {
	public:
		explicit NodeActivation(Scene *app, const ref_ptr<StateNode> &nodeToActivate)
				: app_(app), nodeToActivate_(nodeToActivate) {
		}

		bool interactWith(const ref_ptr<StateNode> &node) override {
			auto rootState = app_->renderTree()->state();
			rootState->enable(RenderState::get());
			nodeToActivate_->traverse(RenderState::get());
			rootState->disable(RenderState::get());
			return true;
		}

	protected:
		Scene *app_;
		ref_ptr<StateNode> nodeToActivate_;
	};

} // regen
