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
				: Animation(true, false),
				  camera_(camera) {}
		// TODO: rather use CPU thread for computation, but it needs synchronization
		//void animate(double dt) override { camera_->updateLight(); }
		void glAnimate(RenderState *rs, double dt) override { camera_->updateLight(); }
	private:
		LightCamera *camera_;
	};
}

// TODO: I am not certain about that the layered rendering is really a good idea,
//       as it does not play well with CPU culling. maybe better to make a draw call
//       for each layer instead?

LightCamera::LightCamera(const ref_ptr<Light> &light, Camera *camera)
		: light_(light), camera_(camera) {
	lightMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightMatrix");
	lightMatrix_->setUniformDataUntyped(nullptr);
	lightCameraAnimation_ = ref_ptr<LightCameraAnimation>::alloc(this);
}
