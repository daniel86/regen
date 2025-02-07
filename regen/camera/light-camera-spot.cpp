#include "light-camera-spot.h"

using namespace regen;

LightCamera_Spot::LightCamera_Spot(const ref_ptr<Light> &light)
		: Camera(1),
		  LightCamera(light, this) {
	setInput(lightMatrix_);
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
		// Transforms world space coordinates to homogenous light space
		lightMatrix_->setVertex(0, viewProj_->getVertex(0) * Mat4f::bias());
		camStamp_ += 1;
		return true;
	}
	return false;
}

bool LightCamera_Spot::updateLightProjection() {
	if (lightRadiusStamp_ == light_->radius()->stamp() &&
		lightConeStamp_ == light_->coneAngle()->stamp()) { return false; }
	const Vec2f &radius = light_->radius()->getVertex(0);
	const Vec2f &coneAngle = light_->coneAngle()->getVertex(0);
	setPerspective(
			1.0f,
			2.0 * acos(coneAngle.y) * RAD_TO_DEGREE,
			lightNear_,
			radius.y);
	lightRadiusStamp_ = light_->radius()->stamp();
	lightConeStamp_ = light_->coneAngle()->stamp();
	return true;
}

bool LightCamera_Spot::updateLightView() {
	if (lightPosStamp_ == light_->position()->stamp() &&
		lightDirStamp_ == light_->direction()->stamp()) { return false; }
	const Vec3f &pos = light_->position()->getVertex(0);
	const Vec3f &dir = light_->direction()->getVertex(0);
	lightPosStamp_ = light_->position()->stamp();
	lightDirStamp_ = light_->direction()->stamp();
	position_->setVertex(0, pos);
	direction_->setVertex(0, dir);
	return updateView();
}
