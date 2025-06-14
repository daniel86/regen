#include "light-camera-parabolic.h"

using namespace regen;

LightCamera_Parabolic::LightCamera_Parabolic(const ref_ptr<Light> &light, bool isDualParabolic)
		: ParabolicCamera(isDualParabolic),
		  LightCamera(light,this) {
	lightMatrix_->set_numArrayElements(numLayer_);
	lightMatrix_->set_forceArray(true);
	lightMatrix_->setUniformUntyped();
	setInput(lightMatrix_);
	updateParabolicLight();
}

bool LightCamera_Parabolic::updateLight() {
	return updateParabolicLight();
}

bool LightCamera_Parabolic::updateParabolicLight() {
	if(updateLightProjection() || updateLightView()) {
		updateViewProjection1();
		// Transforms world space coordinates to homogenous light space
		for (unsigned int i=0; i<lightMatrix_->numArrayElements(); ++i) {
			// note: bias is not applied here, as the projection is done in shaders
			lightMatrix_->setVertex(i, viewProj_->getVertex(i).r);
		}
		camStamp_ += 1;
		return true;
	}
	return false;
}

bool LightCamera_Parabolic::updateLightProjection() {
	if (lightRadiusStamp_ == light_->radius()->stamp()) { return false; }
	auto radius = light_->radius()->getVertex(0);
	lightRadiusStamp_ = light_->radius()->stamp();
	setPerspective(1.0f, 180.0f, lightNear_, radius.r.y);
	return true;
}

bool LightCamera_Parabolic::updateLightView() {
	if (lightPosStamp_ == light_->position()->stamp() &&
		lightDirStamp_ == light_->direction()->stamp()) { return false; }
	auto dir = light_->direction()->getVertex(0);
	lightPosStamp_ = light_->position()->stamp();
	lightDirStamp_ = light_->direction()->stamp();
	position_->setVertex3(0, light_->position()->getVertex(0).r.xyz_());
	direction_->setVertex3(0, -dir.r);
	if (hasBackFace_) {
		direction_->setVertex3(1, dir.r);
	}
	dir.unmap();
	return updateView();
}
