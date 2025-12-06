#ifndef REGEN_PARABOLOID_CAMERA_H
#define REGEN_PARABOLOID_CAMERA_H

#include <regen/camera/camera.h>
#include <regen/camera/light-camera.h>

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

	/**
	 * A camera for a light source with a parabolic projection.
	 * The light source is represented by a parabolic frustum.
	 */
	class LightCamera_Parabolic : public ParabolicCamera, public LightCamera {
	public:
		/**
		 * @param light the light source.
		 * @param isDualParabolic If true use Dual Parabolic.
		 */
		explicit LightCamera_Parabolic(const ref_ptr<Light> &light, bool isDualParabolic = true);

		// override
		bool updateLight() override;

	protected:
		bool updateParabolicLight();
	private:
		bool updateLightProjection();
		bool updateLightView();
	};
} // namespace

#endif /* PARABOLOID_CAMERA_H_ */
