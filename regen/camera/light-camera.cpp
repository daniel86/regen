/*
 * light-camera.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#include <cfloat>

#include "light-camera.h"

using namespace regen;

namespace regen {
	class LightCameraAnimation : public Animation {
	public:
		explicit LightCameraAnimation(LightCamera *camera)
				: Animation(false, true),
				  camera_(camera) {}
		void animate(double dt) override { camera_->updateLight(); }
	private:
		LightCamera *camera_;
	};
}

LightCamera::LightCamera(const ref_ptr<Light> &light, Camera *camera)
		: light_(light), camera_(camera) {
	lightMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightMatrix");
	lightMatrix_->setUniformUntyped();
	lightCameraAnimation_ = ref_ptr<LightCameraAnimation>::alloc(this);
	lightCameraAnimation_->setAnimationName("light-camera");
	lightCameraAnimation_->startAnimation();
}
