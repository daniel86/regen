#ifndef REGEN_CUBE_LIGHT_CAMERA_H
#define REGEN_CUBE_LIGHT_CAMERA_H

#include <regen/camera/light-camera.h>
#include <regen/camera/cube-camera.h>

namespace regen {
	/**
	 * A camera for a point light source represented by a cubic frustum.
	 */
	class LightCamera_Cube : public CubeCamera, public LightCamera {
	public:
		/**
		 * @param light the light source.
		 * @param hiddenFacesMask the mask of hidden faces.
		 */
		explicit LightCamera_Cube(const ref_ptr<Light> &light, int hiddenFacesMask=0);

		// override
		bool updateLight() override;

	protected:
		bool updateCubeLight();
	private:
		bool updateLightProjection();
		bool updateLightView();
	};
} // namespace

#endif /* REGEN_CUBE_LIGHT_CAMERA_H */
