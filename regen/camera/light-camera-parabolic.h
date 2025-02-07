#ifndef REGEN_PARABOLIC_LIGHT_CAMERA_H
#define REGEN_PARABOLIC_LIGHT_CAMERA_H

#include <regen/camera/light-camera.h>
#include <regen/camera/parabolic-camera.h>

namespace regen {
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

#endif /* REGEN_PARABOLIC_LIGHT_CAMERA_H */
