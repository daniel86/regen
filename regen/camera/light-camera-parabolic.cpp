#include "light-camera-parabolic.h"

using namespace regen;

LightCamera_Parabolic::LightCamera_Parabolic(const ref_ptr<Light> &light, bool isDualParabolic)
		: ParabolicCamera(isDualParabolic),
		  LightCamera(light,this) {
	v_lightMatrix_.resize(numLayer_, Mat4f::identity());
	sh_lightMatrix_->set_numArrayElements(numLayer_);
	sh_lightMatrix_->set_forceArray(true);
	sh_lightMatrix_->setUniformUntyped();
	setInput(shadowBuffer_);
	updateParabolicLight();
}

bool LightCamera_Parabolic::updateLight() {
	return updateParabolicLight();
}

bool LightCamera_Parabolic::updateParabolicLight() {
	if(updateLightProjection() || updateLightView()) {
		updateViewProjection1();
		// Transforms world space coordinates to homogenous light space
		for (unsigned int i=0; i<v_lightMatrix_.size(); ++i) {
			// note: bias is not applied here, as the projection is done in shaders
			v_lightMatrix_[i] = viewProjection(i);
		}
		camStamp_ += 1;
		return true;
	}
	return false;
}

bool LightCamera_Parabolic::updateLightProjection() {
	if (lightRadiusStamp_ == light_->radius()->stampOfReadData()) { return false; }
	auto radius = light_->radiusStaged(0);
	lightRadiusStamp_ = light_->radius()->stampOfReadData();
	setPerspective(1.0f, 180.0f, lightNear_, radius.r.y);
	return true;
}

bool LightCamera_Parabolic::updateLightView() {
	if (lightPosStamp_ == light_->position()->stampOfReadData() &&
		lightDirStamp_ == light_->direction()->stampOfReadData()) { return false; }
	auto dir = light_->directionStaged(0);
	lightPosStamp_ = light_->position()->stampOfReadData();
	lightDirStamp_ = light_->direction()->stampOfReadData();
	// Set the position of the light camera
	setPosition(0, light_->positionStaged(0).r.xyz());
	setDirection(0, -dir.r);
	if (hasBackFace_) {
		setDirection(1, dir.r);
	}
	return updateView();
}
