#include "array-camera.h"

using namespace regen;

ArrayCamera::ArrayCamera(unsigned int numLayer)
		: Camera(numLayer) {
	shaderDefine("RENDER_TARGET", "2D_ARRAY");

	// Set matrix array size
	view_->set_numArrayElements(numLayer_);
	view_->set_forceArray(true);
	view_->setUniformUntyped();

	viewInv_->set_numArrayElements(numLayer_);
	viewInv_->set_forceArray(true);
	viewInv_->setUniformUntyped();

	proj_->set_numArrayElements(numLayer_);
	proj_->set_forceArray(true);
	proj_->setUniformUntyped();

	projInv_->set_numArrayElements(numLayer_);
	projInv_->set_forceArray(true);
	projInv_->setUniformUntyped();

	viewProj_->set_numArrayElements(numLayer_);
	viewProj_->set_forceArray(true);
	viewProj_->setUniformUntyped();

	viewProjInv_->set_numArrayElements(numLayer_);
	viewProjInv_->set_forceArray(true);
	viewProjInv_->setUniformUntyped();

	projParams_->set_numArrayElements(numLayer_);
	projParams_->set_forceArray(true);
	projParams_->setUniformUntyped();

	position_->set_numArrayElements(numLayer_);
	position_->set_forceArray(true);
	position_->setUniformUntyped();
	position_->setVertex(0, Vec4f(0.0f));
}
