#include "light-camera-spot.h"

using namespace regen;

LightCamera_Spot::LightCamera_Spot(const ref_ptr<Light> &light)
		: Camera(1, light->lightUBO()->bufferUpdateHints()),
		  LightCamera(light, this) {
	setInput(shadowBuffer_);
	shaderDefine("RENDER_TARGET", "2D");
	// Update matrices
	updateSpotLight();
}

bool LightCamera_Spot::updateLight() {
	return updateSpotLight();
}

bool LightCamera_Spot::updateSpotLight() {
	bool changed = updateLightProjection();
	changed = updateLightView() || changed;
	if(changed) {
		updateViewProjection(0, 0);
		updateFrustumBuffer();
		// Transforms world space coordinates to homogenous light space
		v_lightMatrix_[0] = viewProjection(0) * Mat4f::bias();
		camStamp_ += 1;
		return true;
	}
	return false;
}

bool LightCamera_Spot::updateLightProjection() {
	if (lightRadiusStamp_ == light_->radiusStamp() &&
		lightConeStamp_ == light_->coneAngleStamp()) { return false; }
	auto &radius = light_->radius(0);
	auto &coneAngle = light_->coneAngle(0);
	setPerspective(
			1.0f,
			2.0 * acos(coneAngle.y) * RAD_TO_DEGREE,
			lightNear_,
			radius.y);
	lightRadiusStamp_ = light_->radiusStamp();
	lightConeStamp_ = light_->coneAngleStamp();
	return true;
}

bool LightCamera_Spot::updateLightView() {
	if (lightPosStamp_ == light_->positionStamp() &&
		lightDirStamp_ == light_->directionStamp()) { return false; }
	lightPosStamp_ = light_->positionStamp();
	lightDirStamp_ = light_->directionStamp();
	setPosition(0, light_->position(0).xyz_());
	setDirection(0, light_->direction(0));
	return updateView();
}
