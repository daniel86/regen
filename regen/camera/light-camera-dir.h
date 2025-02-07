#ifndef REGEN_DIR_LIGHT_CAMERA_H
#define REGEN_DIR_LIGHT_CAMERA_H

#include <regen/camera/light-camera.h>
#include <regen/camera/camera.h>

namespace regen {
	/**
	 * A camera for a directional light source.
	 * It uses multiple frustums in a cascade to increase shadow map resolution.
	 * Render target is a 2D array texture.
	 */
	class LightCamera_Directional : public Camera, public LightCamera {
	public:
		/**
		 * @param light the light source.
		 * @param userCamera the user camera.
		 * @param numLayer the number of layers.
		 */
		LightCamera_Directional(
				const ref_ptr<Light> &light,
				const ref_ptr<Camera> &userCamera,
				unsigned int numLayer);

		/**
		 * @param splitWeight the weight of the split between frustums.
		 */
		void setSplitWeight(GLdouble splitWeight);

		// override
		bool updateLight() override;

	protected:
		ref_ptr<Camera> userCamera_;
		std::vector<Frustum> userCameraFrustum_;
		double splitWeight_ = 0.9;

		bool updateDirectionalLight();

	private:
		bool updateLightProjection();
		bool updateLightView();

		unsigned int userProjectionStamp_ = 0u;
		unsigned int userPositionStamp_ = 0u;
		unsigned int userDirectionStamp_ = 0u;
		unsigned int viewStamp_ = 0u;
	};
} // namespace

#endif /* REGEN_DIR_LIGHT_CAMERA_H */
