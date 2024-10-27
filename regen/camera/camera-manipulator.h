/*
 * camera-manipulator.h
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#ifndef CAMERA_MANIPULATOR_H_
#define CAMERA_MANIPULATOR_H_

#include <regen/math/quaternion.h>
#include <regen/camera/camera.h>
#include <regen/animations/animation.h>
#include <regen/animations/bones.h>

namespace regen {
	/**
	 * \brief Computes the view matrix of a Camera.
	 */
	class CameraUpdater {
	public:
		explicit CameraUpdater(const ref_ptr<Camera> &cam);

		void updateCamera(const Vec3f &pos, const Vec3f &dir, GLdouble dt);

	protected:
		ref_ptr<Camera> cam_;

		Mat4f view_;
		Mat4f viewInv_;
		Mat4f viewproj_;
		Mat4f viewprojInv_;
		Vec3f velocity_;
		Vec3f lastPosition_;

		void computeMatrices(const Vec3f &pos, const Vec3f &dir);
	};

	/**
	 * \brief Computes the view matrix of a Camera based on user specified key frames.
	 */
	class KeyFrameCameraTransform : public Animation, public CameraUpdater {
	public:
		explicit KeyFrameCameraTransform(const ref_ptr<Camera> &cam);

		/**
		 * Adds a frame to the list of key frames for camera animation.
		 */
		void push_back(const Vec3f &pos, const Vec3f &dir, GLdouble dt);

		// override
		void animate(GLdouble dt) override;

		void glAnimate(RenderState *rs, GLdouble dt) override;

	protected:
		struct CameraKeyFrame {
			Vec3f pos;
			Vec3f dir;
			GLdouble dt;
		};
		std::list<CameraKeyFrame> frames_;
		std::list<CameraKeyFrame>::iterator it_;
		CameraKeyFrame lastFrame_;
		Vec3f camPos_;
		Vec3f camDir_;
		GLdouble dt_;

		Vec3f interpolate(const Vec3f &v0, const Vec3f &v1, GLdouble t);
	};

	/**
	 * Animation that allows to manipulate a transformation matrix
	 * for first person perspective.
	 */
	class FirstPersonTransform : public Animation {
	public:
		enum PhysicsMode {
			IMPULSE = 0,
			CHARACTER
		};

		explicit FirstPersonTransform(const ref_ptr<ShaderInputMat4> &mat, const ref_ptr<Mesh> &mesh);

		/**
		 * @param v move velocity.
		 */
		void set_moveAmount(GLfloat v);

		/**
		 * @param v moving forward toggle.
		 */
		void moveForward(GLboolean v);

		/**
		 * @param v moving backward toggle.
		 */
		void moveBackward(GLboolean v);

		/**
		 * @param v moving left toggle.
		 */
		void moveLeft(GLboolean v);

		/**
		 * @param v moving right toggle.
		 */
		void moveRight(GLboolean v);

		/**
		 * @param v the amount to step forward.
		 */
		void stepForward(const GLfloat &v);

		/**
		 * @param v the amount to step backward.
		 */
		void stepBackward(const GLfloat &v);

		/**
		 * @param v the amount to step left.
		 */
		void stepLeft(const GLfloat &v);

		/**
		 * @param v the amount to step right.
		 */
		void stepRight(const GLfloat &v);

		/**
		 * @param v the amount to change the position.
		 */
		void step(const Vec3f &v);

		/**
		 * @param v the amount of camera direction change in left direction.
		 */
		void lookLeft(GLdouble v);

		/**
		 * @param v the amount of camera direction change in right direction.
		 */
		void lookRight(GLdouble v);

		void setPhysicsMode(PhysicsMode mode) { physicsMode_ = mode; }

		auto physicsMode() { return physicsMode_; }

		void setPhysicsSpeedFactor(GLfloat factor) { physicsSpeedFactor_ = factor; }

		auto physicsSpeedFactor() { return physicsSpeedFactor_; }

		// override
		void animate(GLdouble dt) override;

		void glAnimate(RenderState *rs, GLdouble dt) override;

	protected:
		ref_ptr<ShaderInputMat4> mat_;
		ref_ptr<Mesh> mesh_;
		Mat4f matVal_;

		Vec3f pos_;
		Vec3f step_;
		//Vec3f dir_;
		GLdouble horizontalOrientation_;
		Vec3f dirXZ_;
		Vec3f dirSidestep_;
		GLfloat moveAmount_;
		Quaternion rot_;

		GLboolean moveForward_;
		GLboolean moveBackward_;
		GLboolean moveLeft_;
		GLboolean moveRight_;
		GLboolean isMoving_;

		PhysicsMode physicsMode_;
		GLfloat physicsSpeedFactor_{};

		void updatePhysicalObject(const ref_ptr<PhysicalObject> &po, GLdouble dt);
	};

	/**
	 * Animation that allows to manipulate a camera together with a mesh
	 * for a first person perspective.
	 */
	class FirstPersonCameraTransform : public FirstPersonTransform, public CameraUpdater {
	public:
		explicit FirstPersonCameraTransform(const ref_ptr<Camera> &cam);

		FirstPersonCameraTransform(
				const ref_ptr<Camera> &cam,
				const ref_ptr<Mesh> &mesh,
				const ref_ptr<ModelTransformation> &transform,
				const Vec3f &meshEyeOffset,
				GLdouble meshHorizontalOrientation);

		/**
		 * @param amount the amount of camera direction change in up direction.
		 */
		void lookUp(GLdouble amount);

		/**
		 * @param amount the amount of camera direction change in down direction.
		 */
		void lookDown(GLdouble amount);

		/**
		 * @param amount the amount to zoom in.
		 */
		virtual void zoomIn(GLdouble amount);

		/**
		 * @param amount the amount to zoom out.
		 */
		virtual void zoomOut(GLdouble amount);

		/**
		 * @param orientation the amount of camera direction change in up direction.
		 */
		void setEyeOrientation(GLdouble orientation) { meshHorizontalOrientation_ = orientation; }

		/**
		 * @param orientation the amount of camera direction change in up direction.
		 */
		void setCameraOrientation(GLdouble orientation) { horizontalOrientation_ = orientation; }

		// override
		void animate(GLdouble dt) override;

		void glAnimate(RenderState *rs, GLdouble dt) override;

	protected:
		ref_ptr<Mesh> mesh_;
		ref_ptr<ShaderInputMat4> mat_;

		Vec3f camPos_;
		Vec3f camDir_;

		GLdouble verticalOrientation_;

		Vec3f meshEyeOffset_;
		GLdouble meshHorizontalOrientation_;

		virtual void updateCameraPosition();

		virtual void updateCameraOrientation();
	};

	/**
	 * Animation that allows to manipulate a camera together with a mesh
	 * for a third person perspective.
	 */
	class ThirdPersonCameraTransform : public FirstPersonCameraTransform {
	public:
		ThirdPersonCameraTransform(
				const ref_ptr<Camera> &cam,
				const ref_ptr<Mesh> &mesh,
				const ref_ptr<ModelTransformation> &transform,
				const Vec3f &eyeOffset,
				GLfloat eyeOrientation);

		// Override
		void zoomIn(GLdouble amount) override;

		void zoomOut(GLdouble amount) override;

	protected:
		Vec3f meshPos_;
		GLfloat meshDistance_;

		void updateCameraPosition() override;

		void updateCameraOrientation() override;
	};
} // namespace

#endif /* CAMERA_MANIPULATOR_H_ */
