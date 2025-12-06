#ifndef REGEN_SPOT_LIGHT_CAMERA_H
#define REGEN_SPOT_LIGHT_CAMERA_H

#include <regen/camera/light-camera.h>
#include <regen/camera/camera.h>

namespace regen {
	/**
	 * A camera for a spot light source.
	 * The light source is represented by a frustum with a perspective projection.
	 */
	class LightCamera_Spot : public Camera, public LightCamera {
	public:
		/**
		 * @param light the light source.
		 */
		explicit LightCamera_Spot(const ref_ptr<Light> &light);

		// override
		bool updateLight() override;

	protected:
		bool updateSpotLight();
	private:
		bool updateLightProjection();
		bool updateLightView();
	};
} // namespace

#endif /* REGEN_SPOT_LIGHT_CAMERA_H */
