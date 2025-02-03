#ifndef REGEN_TRANSFORM_ANIMATION_H
#define REGEN_TRANSFORM_ANIMATION_H

#include <optional>
#include <regen/animations/animation.h>
#include "regen/physics/physical-object.h"
#include "regen/scene/nodes/mesh.h"

namespace regen {
	/**
	 * Transform key frame.
	 */
	struct TransformKeyFrame {
		std::optional<Vec3f> pos;
		std::optional<Vec3f> rotation;
		GLdouble dt;
	};

	/**
	 * Transform animation used to animate model transforms.
	 */
	class TransformAnimation : public Animation {
	public:
		explicit TransformAnimation(const ref_ptr<ShaderInputMat4> &in);

		/**
		 * Push back a key frame.
		 * @param pos the position.
		 * @param rotation the rotation.
		 * @param dt the time difference.
		 */
		void push_back(const std::optional<Vec3f> &pos,
					   const std::optional<Vec3f> &rotation,
					   GLdouble dt);

		/**
		 * Set the mesh, which is used to synchronize with physics
		 * engine in case mesh has a physical object.
		 * @param mesh the mesh.
		 */
		void setMesh(const ref_ptr<Mesh> &mesh) { mesh_ = mesh; }

		// Override Animation
		void animate(GLdouble dt) override;

		// Override Animation
		void glAnimate(RenderState *rs, GLdouble dt) override;

		/**
		 * Update the pose.
		 * @param currentFrame the current frame.
		 * @param t the time.
		 */
		virtual void updatePose(const TransformKeyFrame &currentFrame, double t);

	protected:
		ref_ptr<ShaderInputMat4> in_;
		ref_ptr<Mesh> mesh_;
		std::list<TransformKeyFrame> frames_;
		typename std::list<TransformKeyFrame>::iterator it_;
		TransformKeyFrame lastFrame_;
		GLdouble dt_;
		Vec3f currentPos_;
		Vec3f currentDir_;
		Mat4f currentVal_;
		Vec3f initialScale_;
	};
}

#endif //REGEN_TRANSFORM_ANIMATION_H
