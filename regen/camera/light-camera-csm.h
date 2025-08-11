#ifndef REGEN_CSM_LIGHT_CAMERA_H
#define REGEN_CSM_LIGHT_CAMERA_H

#include <regen/camera/light-camera.h>
#include <regen/camera/camera.h>

namespace regen {
	/**
	 * A camera for a directional light source.
	 * It uses multiple frustums in a cascade to increase shadow map resolution.
	 * Render target is a 2D array texture.
	 */
	class LightCamera_CSM : public Camera, public LightCamera {
	public:
		/**
		 * @param light the light source.
		 * @param userCamera the user camera.
		 * @param numLayer the number of layers.
		 */
		LightCamera_CSM(
				const ref_ptr<Light> &light,
				const ref_ptr<Camera> &userCamera,
				unsigned int numLayer);

		~LightCamera_CSM() override = default;

		/**
		 * @param splitWeight the weight of the split between frustums.
		 */
		void setSplitWeight(GLdouble splitWeight);

		// override
		bool updateLight() override;

	protected:
		ref_ptr<Camera> userCamera_;
		std::vector<Frustum> userCameraFrustum_;
		std::vector<Vec3f> userFrustumCentroids_;
		double splitWeight_ = 0.9;

		bool updateDirectionalLight();

	private:
		bool updateLightProjection();
		bool updateLightView();
		bool updateFrustumSplit();

		unsigned int userProjectionStamp_ = 0u;
		unsigned int userPositionStamp_ = 0u;
		unsigned int userDirectionStamp_ = 0u;
		unsigned int viewStamp_ = 0u;
	};
} // namespace

#endif /* REGEN_CSM_LIGHT_CAMERA_H */
