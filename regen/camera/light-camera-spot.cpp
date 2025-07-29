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
	if (lightRadiusStamp_ == light_->radius()->stamp() &&
		lightConeStamp_ == light_->coneAngle()->stamp()) { return false; }
	auto radius = light_->radius()->getVertex(0);
	auto coneAngle = light_->coneAngle()->getVertex(0);
	setPerspective(
			1.0f,
			2.0 * acos(coneAngle.r.y) * RAD_TO_DEGREE,
			lightNear_,
			radius.r.y);
	lightRadiusStamp_ = light_->radius()->stamp();
	lightConeStamp_ = light_->coneAngle()->stamp();
	return true;
}

bool LightCamera_Spot::updateLightView() {
	if (lightPosStamp_ == light_->position()->stamp() &&
		lightDirStamp_ == light_->direction()->stamp()) { return false; }
	lightPosStamp_ = light_->position()->stamp();
	lightDirStamp_ = light_->direction()->stamp();
	setPosition(0, light_->position()->getVertex(0).r.xyz_());
	setDirection(0, light_->direction()->getVertex(0).r);
	return updateView();
}
