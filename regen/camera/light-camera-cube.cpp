#include "light-camera-cube.h"

using namespace regen;

LightCamera_Cube::LightCamera_Cube(const ref_ptr<Light> &light, int hiddenFacesMask)
		: CubeCamera(hiddenFacesMask),
		  LightCamera(light,this) {
	v_lightMatrix_.resize(numLayer_, Mat4f::identity());
	sh_lightMatrix_->set_numArrayElements(numLayer_);
	sh_lightMatrix_->set_forceArray(true);
	sh_lightMatrix_->setUniformUntyped();
	setInput(shadowBuffer_);
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
				v_lightMatrix_[i] = viewProjection(i) * Mat4f::bias();
			}
		}
		camStamp_ += 1;
		return true;
	}
	return false;
}

bool LightCamera_Cube::updateLightProjection() {
	if (lightRadiusStamp_ == light_->radius()->stampOfReadData()) { return false; }
	lightRadiusStamp_ = light_->radius()->stampOfReadData();

	auto radius = light_->radiusStaged(0);
	setPerspective(1.0f, 90.0f, lightNear_, radius.r.y);
	return true;
}

bool LightCamera_Cube::updateLightView() {
	if (lightPosStamp_ == light_->position()->stampOfReadData()) { return false; }
	lightPosStamp_ = light_->position()->stampOfReadData();
	setPosition(0, light_->positionStaged(0).r.xyz());
	return updateView();
}
