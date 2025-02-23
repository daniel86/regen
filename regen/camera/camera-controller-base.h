#ifndef REGEN_CAMERA_CONTROLLER_BASE_H
#define REGEN_CAMERA_CONTROLLER_BASE_H

#include <regen/camera/camera.h>

namespace regen {
	/**
	 * \brief Computes the transformation matrices of a Camera.
	 */
	class CameraControllerBase {
	public:
		explicit CameraControllerBase(const ref_ptr<Camera> &cam);

		virtual ~CameraControllerBase() = default;

		/**
		 * Computes the transformation matrices of the camera.
		 * @param pos the camera position.
		 * @param dir the camera direction.
		 * @param dt the time step.
		 */
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
} // namespace

#endif //REGEN_CAMERA_CONTROLLER_BASE_H
