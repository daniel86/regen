#ifndef REGEN_ATTRIBUTE_FEEDBACK_ANIMATION_H
#define REGEN_ATTRIBUTE_FEEDBACK_ANIMATION_H

#include <regen/animation/animation.h>
#include "regen/states/state-node.h"
#include "regen/shader/shader-state.h"
#include "regen/states/feedback-state.h"

namespace regen {
	/**
	 * \brief Animation that uses transform feedback to animate a mesh.
	 */
	class AttributeFeedbackAnimation : public Animation {
	public:
		/**
		 * Constructor.
		 * @param inputMesh the mesh to animate.
		 * @param shaderKey the shader key to use for the animation.
		 */
		AttributeFeedbackAnimation(const ref_ptr<Mesh> &inputMesh, const std::string &shaderKey);

		~AttributeFeedbackAnimation() override = default;

		/**
		 * Initialize the transform feedback buffer and the shader state.
		 */
		void initializeResources();

		// Override
		void gpuUpdate(regen::RenderState *rs, double dt) override;

		/**
		 * Create a new animation node.
		 * @param scene the scene to add the animation to.
		 * @param input the input node to add the animation to.
		 * @return the new animation node.
		 */
		static ref_ptr<AttributeFeedbackAnimation> load(scene::SceneLoader *scene, scene::SceneInputNode &input);

	protected:
		ref_ptr<StateNode> animationNode_;
		ref_ptr<Mesh> inputMesh_;
		ref_ptr<Mesh> feedbackMesh_;
		ref_ptr<FeedbackState> feedbackState_;
		ref_ptr<ShaderState> shaderState_;
		const std::string shaderKey_;

		std::vector<NamedShaderInput> originalAttributes_;
		std::vector<NamedShaderInput> feedbackAttributes_;
		uint32_t bufferSize_;
	};
}

#endif //REGEN_ATTRIBUTE_FEEDBACK_H
