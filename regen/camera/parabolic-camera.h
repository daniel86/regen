#ifndef REGEN_PARABOLOID_CAMERA_H
#define REGEN_PARABOLOID_CAMERA_H

#include <regen/camera/camera.h>
#include <regen/meshes/mesh-state.h>

namespace regen {
	/**
	 * A camera with a parabolic projection
	 * computed in shaders.
	 */
	class ParabolicCamera : public Camera {
	public:
		/**
		 * @param hasBackFace If true use Dual Parabolic.
		 */
		explicit ParabolicCamera(bool isDualParabolic = true);

		/**
		 * @return If true the camera has a back face.
		 */
		bool isDualParabolic() const { return hasBackFace_; }

		/**
		 * @return If true the camera has a single parabolic projection.
		 */
		bool isSingleParabolic() const { return !hasBackFace_; }

		/**
		 * Set the normal of the parabolic projection.
		 * @param normal The normal.
		 */
		void setNormal(const Vec3f &normal);

	protected:
		bool hasBackFace_;

		void updateViewProjection(unsigned int projectionIndex, unsigned int viewIndex) override;
	};
} // namespace

#endif /* PARABOLOID_CAMERA_H_ */
