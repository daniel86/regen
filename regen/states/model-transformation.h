/*
 * model-transformation.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef MODEL_TRANSFORMATION_H_
#define MODEL_TRANSFORMATION_H_

#include <regen/av/audio.h>
#include <regen/math/quaternion.h>
#include <regen/gl-types/shader-input-container.h>
#include <regen/states/state.h>

namespace regen {
	/**
	 * \brief matrix that transforms for model space to world space.
	 *
	 * Usually meshes should be defined at origin and then translated
	 * and rotated to the world position.
	 */
	class ModelTransformation : public State, public HasInput {
	public:
		static constexpr const char *TYPE_NAME = "ModelTransformation";

		ModelTransformation();

		static ref_ptr<ModelTransformation> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<State> &state);

		/**
		 * @return the model transformation matrix.
		 */
		const ref_ptr<ShaderInputMat4> &get() const;

		/**
		 * @param audioSource the audio source attached to the world position
		 * of the model.
		 */
		void set_audioSource(const ref_ptr<AudioSource> &audioSource);

		/**
		 * @return the audio source attached to the world position
		 * of the model.
		 */
		GLboolean isAudioSource() const;

		// Override
		void enable(RenderState *rs) override;

	protected:
		ref_ptr<ShaderInputMat4> modelMat_;
		ref_ptr<ShaderInput3f> velocity_;

		ref_ptr<AudioSource> audioSource_;

		Vec3f lastPosition_;
		boost::posix_time::ptime lastTime_;
	};
} // namespace

#endif /* MODEL_TRANSFORMATION_H_ */
