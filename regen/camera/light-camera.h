/*
 * light-camera.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

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
		 * @return A matrix used to transform world space points to
		 *          texture coordinates for shadow mapping.
		 */
		auto &lightMatrix() const { return lightMatrix_; }

		/**
		 * @return The light camera.
		 */
		auto &lightCamera() const { return camera_; }

		/**
		 * @param near the near plane distance.
		 */
		void setLightNear(float near) { lightNear_ = near; }

		/**
		 * @return the near plane distance.
		 */
		auto lightNear() const { return lightNear_; }

		/**
		 * Update the light camera.
		 * @return True if the light camera was updated.
		 */
		virtual bool updateLight() = 0;

	protected:
		ref_ptr<Light> light_;
		Camera *camera_;
		ref_ptr<ShaderInputMat4> lightMatrix_;
		ref_ptr<Animation> lightCameraAnimation_;

		float lightNear_ = 0.1f;

		unsigned int lightPosStamp_ = 0u;
		unsigned int lightDirStamp_ = 0u;
		unsigned int lightRadiusStamp_ = 0u;
		unsigned int lightConeStamp_ = 0u;
	};
} // namespace

#endif /* REGEN_LIGHT_CAMERA_H */
