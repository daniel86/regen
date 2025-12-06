#include "spot-light-camera.h"

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
	if (lightRadiusStamp_ == light_->radius()->stampOfReadData() &&
		lightConeStamp_ == light_->coneAngle()->stampOfReadData()) { return false; }
	const auto radius = light_->radiusStaged(0);
	const auto coneAngle = light_->coneAngleStaged(0);
	setPerspective(
			1.0f,
			2.0f * acosf(coneAngle.r.y) * math::RAD_TO_DEG,
			lightNear_,
			radius.r.y);
	lightRadiusStamp_ = light_->radius()->stampOfReadData();
	lightConeStamp_ = light_->coneAngle()->stampOfReadData();
	return true;
}

bool LightCamera_Spot::updateLightView() {
	if (lightPosStamp_ == light_->position()->stampOfReadData() &&
		lightDirStamp_ == light_->direction()->stampOfReadData()) { return false; }
	lightPosStamp_ = light_->position()->stampOfReadData();
	lightDirStamp_ = light_->direction()->stampOfReadData();
	setPosition(0, light_->positionStaged(0).r.xyz());
	setDirection(0, light_->directionStaged(0).r);
	return updateView();
}
