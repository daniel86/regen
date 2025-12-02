#ifndef REGEN_TRANSFORM_ANIMATION_H
#define REGEN_TRANSFORM_ANIMATION_H

#include <optional>
#include <regen/animation/animation.h>
#include "regen/simulation/physical-object.h"
#include "regen/scene/mesh-processor.h"

namespace regen {
	/**
	 * Transform key frame.
	 */
	struct TransformKeyFrame {
		std::optional<Vec3f> pos = std::nullopt;
		std::optional<Vec3f> rotation = std::nullopt;
		double dt = 0.0;
	};

	/**
	 * Transform animation used to animate model transforms.
	 */
	class TransformAnimation : public Animation {
	public:
		explicit TransformAnimation(const ref_ptr<ModelTransformation> &tf, uint32_t tfIdx);

		~TransformAnimation() override = default;

		/**
		 * Set if the TF animation should loop, i.e. once reaching
		 * the last key frame, it continues with the first one.
		 * @param loop true to loop, false otherwise.
		 */
		void setLoopTransformAnimation(bool loop) { loopTransformAnimation_ = loop; }

		/**
		 * Push back a key frame.
		 * @param pos the position.
		 * @param rotation the rotation.
		 * @param dt the time difference.
		 */
		void addTransformKeyframe(
			const std::optional<Vec3f> &pos,
			const std::optional<Vec3f> &rotation,
			double dt);

		/**
		 * Set the mesh, which is used to synchronize with physics
		 * engine in case mesh has a physical object.
		 * @param mesh the mesh.
		 */
		void setMesh(const ref_ptr<Mesh> &mesh) { mesh_ = mesh; }

		// Override Animation
		void cpuUpdate(double dt) override;

		/**
		 * Update the pose.
		 * @param currentFrame the current frame.
		 * @param t the time.
		 */
		virtual void updatePose(const TransformKeyFrame &currentFrame, double t);

	protected:
		ref_ptr<ModelTransformation> tf_;
		uint32_t tfIdx_;
		ref_ptr<Mesh> mesh_;
		std::list<TransformKeyFrame> frames_;
		typename std::list<TransformKeyFrame>::iterator it_;
		TransformKeyFrame lastFrame_;
		double dt_;
		Vec3f currentPos_;
		Vec3f currentVel_ = Vec3f::zero();
		Vec3f currentDir_;
		Mat4f currentVal_;
		Vec3f initialScale_;
		bool loopTransformAnimation_ = false;
	};
}

#endif //REGEN_TRANSFORM_ANIMATION_H
