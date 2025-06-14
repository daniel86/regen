#include "light-camera-cube.h"

using namespace regen;

LightCamera_Cube::LightCamera_Cube(const ref_ptr<Light> &light, int hiddenFacesMask)
		: CubeCamera(hiddenFacesMask),
		  LightCamera(light,this) {
	lightMatrix_->set_numArrayElements(numLayer_);
	lightMatrix_->set_forceArray(true);
	lightMatrix_->setUniformUntyped();
	setInput(lightMatrix_);
	updateCubeLight();
}

bool LightCamera_Cube::updateLight() {
	return updateCubeLight();
}

bool LightCamera_Cube::updateCubeLight() {
	bool changed = updateLightProjection();
	changed = updateLightView() || changed;
	if(changed) {
		updateViewProjection1();
		// Transforms world space coordinates to homogenous light space
		for (auto i=0; i<6; ++i) {
			if (isCubeFaceVisible(i)) {
				lightMatrix_->setVertex(i, viewProj_->getVertex(i).r * Mat4f::bias());
			}
		}
		camStamp_ += 1;
		return true;
	}
	return false;
}

bool LightCamera_Cube::updateLightProjection() {
	if (lightRadiusStamp_ == light_->radius()->stamp()) { return false; }
	lightRadiusStamp_ = light_->radius()->stamp();

	auto radius = light_->radius()->getVertex(0);
	setPerspective(1.0f, 90.0f, lightNear_, radius.r.y);
	return true;
}

bool LightCamera_Cube::updateLightView() {
	if (lightPosStamp_ == light_->position()->stamp()) { return false; }
	lightPosStamp_ = light_->position()->stamp();
	position_->setVertex3(0,
		light_->position()->getVertex(0).r.xyz_());
	return updateView();
}
