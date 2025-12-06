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
		void setSplitWeight(double splitWeight) { splitWeight_ = splitWeight; }

		/**
		 * @param depthPadding the padding to add to the near and far planes in light space.
		 */
		void setDepthPadding(double depthPadding) { depthPadding_ = depthPadding; }

		/**
		 * @param orthoPadding the padding to add to the orthographic projection in light space.
		 */
		void setOrthoPadding(double orthoPadding) { orthoPadding_ = orthoPadding; }

		/**
		 * This is less efficient, but avoids some artifacts when sampling between cascades.
		 * @param useUniformDepthRange if true, all frustums use the same depth range.
		 */
		void setUseUniformDepthRange(bool useUniformDepthRange) { useUniformDepthRange_ = useUniformDepthRange; }

		// override
		bool updateLight() override;

	protected:
		ref_ptr<Camera> userCamera_;
		std::vector<Frustum> userCameraFrustum_;
		std::vector<Vec3f> userFrustumCentroids_;
		std::vector<Bounds<Vec3f>> lightSpaceBounds_;
		double splitWeight_ = 0.9;
		double depthPadding_ = 0.0;
		double orthoPadding_ = 0.0;
		bool useUniformDepthRange_ = true;

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
