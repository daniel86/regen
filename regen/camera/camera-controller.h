#ifndef CAMERA_MANIPULATOR_H_
#define CAMERA_MANIPULATOR_H_

#include <regen/compute/quaternion.h>
#include <regen/camera/camera.h>
#include <regen/camera/camera-controller-base.h>
#include <regen/animation/animation.h>

namespace regen {
	/**
	 * Camera commands.
	 */
	enum CameraCommand {
		MOVE_FORWARD = 0,
		MOVE_BACKWARD,
		MOVE_LEFT,
		MOVE_RIGHT,
		MOVE_UP,
		MOVE_DOWN,
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

		~CameraController() override = default;

		/**
		 * Initializes the camera controller.
		 */
		void initCameraController();

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
		void setAttachedTo(const ref_ptr<ModelTransformation> &target, const ref_ptr<Mesh> &mesh);

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
		void set_moveAmount(float v) { moveAmount_ = v; }

		/**
		 * @param v moving forward toggle.
		 */
		void moveForward(bool v) { moveForward_ = v; }

		/**
		 * @param v moving backward toggle.
		 */
		void moveBackward(bool v) { moveBackward_ = v; }

		/**
		 * @param v moving left toggle.
		 */
		void moveLeft(bool v) { moveLeft_ = v; }

		/**
		 * @param v moving right toggle.
		 */
		void moveRight(bool v) { moveRight_ = v; }

		/**
		 * @param v moving up toggle.
		 */
		void moveUp(bool v) { moveUp_ = v; }

		/**
		 * @param v moving down toggle.
		 */
		void moveDown(bool v) { moveDown_ = v; }

		/**
		 * @param v the amount to step forward.
		 */
		void stepForward(const float &v);

		/**
		 * @param v the amount to step backward.
		 */
		void stepBackward(const float &v);

		/**
		 * @param v the amount to step left.
		 */
		void stepLeft(const float &v);

		/**
		 * @param v the amount to step right.
		 */
		void stepRight(const float &v);

		/**
		 * @param v the amount to step up.
		 */
		void stepUp(const float &v);

		/**
		 * @param v the amount to step down.
		 */
		void stepDown(const float &v);

		/**
		 * @param v the amount to change the position.
		 */
		void step(const Vec3f &v);

		/**
		 * @param v the amount of camera direction change in left direction.
		 */
		void lookLeft(double v);

		/**
		 * @param v the amount of camera direction change in right direction.
		 */
		void lookRight(double v);

		/**
		 * @param amount the amount of camera direction change in up direction.
		 */
		void lookUp(double amount);

		/**
		 * @param amount the amount of camera direction change in down direction.
		 */
		void lookDown(double amount);

		/**
		 * @param amount the amount to zoom in.
		 */
		void zoomIn(double amount);

		/**
		 * @param amount the amount to zoom out.
		 */
		void zoomOut(double amount);

		/**
		 * @param orientation the orientation of the camera.
		 */
		void setHorizontalOrientation(double orientation) { horizontalOrientation_ = orientation; }

		/**
		 * @param orientation the orientation of the camera.
		 */
		void setVerticalOrientation(double orientation) { verticalOrientation_ = orientation; }

		/**
		 * @param distance the distance to the mesh.
		 * @note only has an effect in third person mode.
		 */
		void setMeshDistance(float distance) { meshDistance_ = distance; }

		/**
		 * @param offset the offset to the "eye position" of the mesh.
		 * @note only has an effect if mesh is set.
		 */
		void setMeshEyeOffset(const Vec3f &offset) { meshEyeOffset_ = offset; }

		/**
		 * @param orientation initial orientation of the mesh relative to the camera.
		 */
		void setMeshHorizontalOrientation(double orientation) { meshHorizontalOrientation_ = orientation; }

		/**
		 * Jump.
		 */
		virtual void jump();

		// override Animation
		void cpuUpdate(double dt) override;

	protected:
		Mode cameraMode_ = FIRST_PERSON;
		Mat4f matVal_ = Mat4f::identity();
		Vec3f camPos_ = Vec3f::zero();
		Vec3f camDir_ = Vec3f::front();

		ref_ptr<ModelTransformation> attachedToTransform_;
		ref_ptr<Mesh> attachedToMesh_;
		Vec3f meshPos_ = Vec3f::zero();
		float meshDistance_ = 10.0f;

		Vec3f pos_ = Vec3f::zero();
		Vec3f step_ = Vec3f::zero();
		double horizontalOrientation_ = 0.0;
		double verticalOrientation_ = 0.0;
		float orientThreshold_;
		Vec3f dirXZ_ = Vec3f::front();
		Vec3f dirSidestep_ = Vec3f::right();
		float moveAmount_ = 1.0;
		Quaternion rot_;

		bool moveForward_ = false;
		bool moveBackward_ = false;
		bool moveLeft_ = false;
		bool moveRight_ = false;
		bool moveUp_ = false;
		bool moveDown_ = false;
		bool isMoving_ = false;
		bool isRotating_ = false;
		double lastOrientation_ = -1.0;

		Vec3f meshEyeOffset_ = Vec3f::zero();
		double meshHorizontalOrientation_ = 0.0;
		bool hasUpdated_ = false;

		void updateStep(double dt);

		void updateCameraPose();
	};
} // namespace

#endif /* CAMERA_MANIPULATOR_H_ */
