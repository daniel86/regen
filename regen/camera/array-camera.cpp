#include "array-camera.h"

using namespace regen;

ArrayCamera::ArrayCamera(unsigned int numLayer, const BufferUpdateFlags &updateFlags)
		: Camera(numLayer, updateFlags) {
	shaderDefine("RENDER_TARGET", "2D_ARRAY");

	view_.resize(numLayer, Mat4f::identity());
	sh_view_->set_numArrayElements(numLayer_);
	sh_view_->set_forceArray(true);
	sh_view_->setUniformUntyped();

	viewInv_.resize(numLayer, Mat4f::identity());
	sh_viewInv_->set_numArrayElements(numLayer_);
	sh_viewInv_->set_forceArray(true);
	sh_viewInv_->setUniformUntyped();

	proj_.resize(numLayer, Mat4f::identity());
	sh_proj_->set_numArrayElements(numLayer_);
	sh_proj_->set_forceArray(true);
	sh_proj_->setUniformUntyped();

	projInv_.resize(numLayer, Mat4f::identity());
	sh_projInv_->set_numArrayElements(numLayer_);
	sh_projInv_->set_forceArray(true);
	sh_projInv_->setUniformUntyped();

	viewProj_.resize(numLayer, Mat4f::identity());
	sh_viewProj_->set_numArrayElements(numLayer_);
	sh_viewProj_->set_forceArray(true);
	sh_viewProj_->setUniformUntyped();

	viewProjInv_.resize(numLayer, Mat4f::identity());
	sh_viewProjInv_->set_numArrayElements(numLayer_);
	sh_viewProjInv_->set_forceArray(true);
	sh_viewProjInv_->setUniformUntyped();

	projParams_.resize(numLayer);
	sh_projParams_->set_numArrayElements(numLayer_);
	sh_projParams_->set_forceArray(true);
	sh_projParams_->setUniformUntyped();

	position_.resize(numLayer, Vec4f::zero());
	sh_position_->set_numArrayElements(numLayer_);
	sh_position_->set_forceArray(true);
	sh_position_->setUniformUntyped();
	sh_position_->setVertex(0, Vec4f(0.0f));
}
