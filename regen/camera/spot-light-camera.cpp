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
	const uint32_t radiusStamp = light_->radius()->stampOfReadData();
	const uint32_t coneStamp = light_->coneAngle()->stampOfReadData();
	if (lightRadiusStamp_ == radiusStamp && lightConeStamp_ == coneStamp) { return false; }
	const auto radius = light_->radiusStaged(0);
	const auto coneAngle = light_->coneAngleStaged(0);
	setPerspective(
			1.0f,
			2.0f * acosf(coneAngle.r.y) * math::RAD_TO_DEG,
			lightNear_,
			radius.r.y);
	lightRadiusStamp_ = radiusStamp;
	lightConeStamp_ = coneStamp;
	return true;
}

bool LightCamera_Spot::updateLightView() {
	const uint32_t posStamp = light_->position()->stampOfReadData();
	const uint32_t dirStamp = light_->direction()->stampOfReadData();
	if (lightPosStamp_ == posStamp && lightDirStamp_ == dirStamp) { return false; }
	lightPosStamp_ = posStamp;
	lightDirStamp_ = dirStamp;
	setPosition(0, light_->positionStaged(0).r.xyz());
	setDirection(0, light_->directionStaged(0).r);
	return updateView();
}
