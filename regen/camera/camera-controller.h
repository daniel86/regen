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
#include <regen/camera/camera-controller-base.h>
#include <regen/animations/animation.h>
#include <regen/animations/bones.h>

namespace regen {
	/**
	 * Camera commands.
	 */
	enum CameraCommand {
		MOVE_FORWARD = 0,
		MOVE_BACKWARD,
		MOVE_LEFT,
		MOVE_RIGHT,
		JUMP,
		CROUCH,
		NONE
	};

	std::ostream &operator<<(std::ostream &out, const CameraCommand &v);

	std::istream &operator>>(std::istream &in, CameraCommand &v);

	/**
	 * Mapping of keys to camera commands.
	 */
	struct CameraCommandMapping {
		std::string key;
		CameraCommand command;
	};

	/**
	 * Animation that allows to manipulate a transformation matrix
	 * for first person or third person perspective.
	 */
	class CameraController : public Animation, public CameraControllerBase {
	public:
		/**
		 * Camera modes.
		 */
		enum Mode {
			FIRST_PERSON = 0,
			THIRD_PERSON
		};

		/**
		 * Default constructor.
		 * @param cam the camera.
		 */
		explicit CameraController(const ref_ptr<Camera> &cam);

		/**
		 * Initializes the camera with a position and direction.
		 * @param pos the position of the camera.
		 * @param dir the direction of the camera.
		 */
		void setTransform(const Vec3f &pos, const Vec3f &dir);

		/**
		 * Attaches the camera to a transformation matrix.
		 * @param target the transformation matrix.
		 * @param mesh the optional mesh to attach to.
		 */
		void setAttachedTo(const ref_ptr<ShaderInputMat4> &target, const ref_ptr<Mesh> &mesh);

		/**
		 * @return true if the camera is in first person mode.
		 */
		bool isFirstPerson() const { return cameraMode_ == FIRST_PERSON; }

		/**
		 * @return true if the camera is in third person mode.
		 */
		bool isThirdPerson() const { return cameraMode_ == THIRD_PERSON; }

		/**
		 * @param mode the camera mode.
		 */
		void setCameraMode(Mode mode) { cameraMode_ = mode; }

		/**
		 * @param v move velocity.
		 */
		void set_moveAmount(GLfloat v) { moveAmount_ = v; }

		/**
		 * @param v moving forward toggle.
		 */
		void moveForward(GLboolean v) { moveForward_ = v; }

		/**
		 * @param v moving backward toggle.
		 */
		void moveBackward(GLboolean v) { moveBackward_ = v; }

		/**
		 * @param v moving left toggle.
		 */
		void moveLeft(GLboolean v) { moveLeft_ = v; }

		/**
		 * @param v moving right toggle.
		 */
		void moveRight(GLboolean v) { moveRight_ = v; }

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
		void zoomIn(GLdouble amount);

		/**
		 * @param amount the amount to zoom out.
		 */
		void zoomOut(GLdouble amount);

		/**
		 * @param orientation the orientation of the camera.
		 */
		void setHorizontalOrientation(GLdouble orientation) { horizontalOrientation_ = orientation; }

		/**
		 * @param orientation the orientation of the camera.
		 */
		void setVerticalOrientation(GLdouble orientation) { verticalOrientation_ = orientation; }

		/**
		 * @param distance the distance to the mesh.
		 * @note only has an effect in third person mode.
		 */
		void setMeshDistance(GLfloat distance) { meshDistance_ = distance; }

		/**
		 * @param offset the offset to the "eye position" of the mesh.
		 * @note only has an effect if mesh is set.
		 */
		void setMeshEyeOffset(const Vec3f &offset) { meshEyeOffset_ = offset; }

		/**
		 * @param orientation initial orientation of the mesh relative to the camera.
		 */
		void setMeshHorizontalOrientation(GLdouble orientation) { meshHorizontalOrientation_ = orientation; }

		/**
		 * Move a step in the direction of the given offset.
		 * @param offset the offset.
		 */
		virtual void applyStep(GLfloat dt, const Vec3f &offset);

		/**
		 * Jump.
		 */
		virtual void jump();

		// override Animation
		void animate(GLdouble dt) override;

		// override Animation
		void glAnimate(RenderState *rs, GLdouble dt) override;

	protected:
		Mode cameraMode_;
		Mat4f matVal_;
		Vec3f camPos_;
		Vec3f camDir_;

		ref_ptr<ShaderInputMat4> attachedToTransform_;
		ref_ptr<Mesh> attachedToMesh_;
		Vec3f meshPos_;
		GLfloat meshDistance_;

		Vec3f pos_;
		Vec3f step_;
		//Vec3f dir_;
		GLdouble horizontalOrientation_;
		GLdouble verticalOrientation_;
		GLfloat orientThreshold_;
		Vec3f dirXZ_;
		Vec3f dirSidestep_;
		GLfloat moveAmount_;
		Quaternion rot_;

		GLboolean moveForward_;
		GLboolean moveBackward_;
		GLboolean moveLeft_;
		GLboolean moveRight_;
		GLboolean isMoving_;

		Vec3f meshEyeOffset_;
		GLdouble meshHorizontalOrientation_;

		void updateCameraPosition();
		void updateCameraOrientation();
		void updateModel();
	};
} // namespace

#endif /* CAMERA_MANIPULATOR_H_ */
