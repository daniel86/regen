#ifndef REGEN_LIGHT_CAMERA_H
#define REGEN_LIGHT_CAMERA_H

#include <regen/states/light-state.h>
#include <regen/camera/camera.h>

namespace regen {
	/**
	 * A camera for a light source.
	 */
	class LightCamera {
	public:
		/**
		 * @param light the light source.
		 * @param camera the camera.
		 */
		LightCamera(const ref_ptr<Light> &light, Camera *camera);

		virtual ~LightCamera() = default;

		/**
		 * The shadow buffer for the light camera stores the light matrix.
		 * @return The shadow buffer.
		 */
		const ref_ptr<UBO> &shadowBuffer() const { return shadowBuffer_; }

		/**
		 * The light matrix transforms world space points to
		 * texture coordinates for shadow mapping.
		 * @return The light matrix.
		 */
		const std::vector<Mat4f> &lightMatrix() const { return v_lightMatrix_; }

		/**
		 * Get the light matrix for a specific layer.
		 * @param idx the layer index.
		 * @return The light matrix for the layer.
		 */
		const Mat4f &lightMatrix(uint32_t idx) const { return v_lightMatrix_[idx]; }

		/**
		 * @return The light matrix shader input.
		 */
		const ref_ptr<ShaderInputMat4> &sh_lightMatrix() const { return sh_lightMatrix_; }

		/**
		 * @return The light camera.
		 */
		Camera* lightCamera() { return camera_; }

		/**
		 * @param near the near plane distance.
		 */
		void setLightNear(float near) { lightNear_ = near; }

		/**
		 * @return the near plane distance.
		 */
		float lightNear() const { return lightNear_; }

		/**
		 * Update the light camera.
		 * @return True if the light camera was updated.
		 */
		virtual bool updateLight() = 0;

		/**
		 * Update the shadow shader data.
		 */
		void updateShadowData();

	protected:
		ref_ptr<Light> light_;
		Camera *camera_;
		ref_ptr<Animation> lightCameraAnimation_;

		ref_ptr<UBO> shadowBuffer_;
		ref_ptr<ShaderInputMat4> sh_lightMatrix_;
		std::vector<Mat4f> v_lightMatrix_;

		float lightNear_ = 0.1f;

		unsigned int lightPosStamp_ = 0u;
		unsigned int lightDirStamp_ = 0u;
		unsigned int lightRadiusStamp_ = 0u;
		unsigned int lightConeStamp_ = 0u;
	};
} // namespace

#endif /* REGEN_LIGHT_CAMERA_H */
