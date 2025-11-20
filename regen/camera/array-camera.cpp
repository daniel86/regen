#include "array-camera.h"

using namespace regen;

ArrayCamera::ArrayCamera(unsigned int numLayer, const BufferUpdateFlags &updateFlags)
		: Camera(numLayer, updateFlags) {
	shaderDefine("RENDER_TARGET", "2D_ARRAY");

	{
		viewData_.resize(numLayer*2, Mat4f::identity());
		view_    = std::span<Mat4f>(viewData_).subspan(0, numLayer);
		viewInv_ = std::span<Mat4f>(viewData_).subspan(numLayer, numLayer);

		sh_view_->set_numArrayElements(numLayer_);
		sh_viewInv_->set_numArrayElements(numLayer_);
		sh_view_->set_forceArray(true);
		sh_viewInv_->set_forceArray(true);
		sh_view_->setUniformUntyped();
		sh_viewInv_->setUniformUntyped();
	}
	{
		viewProjData_.resize(numLayer*2, Mat4f::identity());
		viewProj_    = std::span<Mat4f>(viewProjData_).subspan(0, numLayer);
		viewProjInv_ = std::span<Mat4f>(viewProjData_).subspan(numLayer, numLayer);

		sh_viewProj_->set_numArrayElements(numLayer_);
		sh_viewProjInv_->set_numArrayElements(numLayer_);
		sh_viewProj_->set_forceArray(true);
		sh_viewProjInv_->set_forceArray(true);
		sh_viewProj_->setUniformUntyped();
		sh_viewProjInv_->setUniformUntyped();
	}
	{
		projData_.resize(numLayer*2, Mat4f::identity());
		proj_    = std::span<Mat4f>(projData_).subspan(0, numLayer);
		projInv_ = std::span<Mat4f>(projData_).subspan(numLayer, numLayer);

		sh_proj_->set_numArrayElements(numLayer_);
		sh_projInv_->set_numArrayElements(numLayer_);
		sh_proj_->set_forceArray(true);
		sh_projInv_->set_forceArray(true);
		sh_proj_->setUniformUntyped();
		sh_projInv_->setUniformUntyped();
	}

	projParams_.resize(numLayer);
	sh_projParams_->set_numArrayElements(numLayer_);
	sh_projParams_->set_forceArray(true);
	sh_projParams_->setUniformUntyped();

	position_.resize(numLayer, Vec4f::zero());
	sh_position_->set_numArrayElements(numLayer_);
	sh_position_->set_forceArray(true);
	sh_position_->setUniformUntyped();
	sh_position_->setVertex(0, Vec4f::create(0.0f));
}
