#include "parabolic-camera.h"

using namespace regen;

ParabolicCamera::ParabolicCamera(bool isDualParabolic)
		: Camera((isDualParabolic ? 2 : 1)),
		  hasBackFace_(isDualParabolic) {
	shaderDefine("RENDER_TARGET", isDualParabolic ? "DUAL_PARABOLOID" : "PARABOLOID");
	shaderDefine("USE_PARABOLOID_PROJECTION", "TRUE");
	isOmni_ = true;

	{
		viewData_.resize(numLayer_ * 2, Mat4f::identity());
		view_    = std::span<Mat4f>(viewData_).subspan(0, numLayer_);
		viewInv_ = std::span<Mat4f>(viewData_).subspan(numLayer_, numLayer_);
		sh_view_->set_numArrayElements(numLayer_);
		sh_viewInv_->set_numArrayElements(numLayer_);
		sh_view_->setUniformUntyped();
		sh_viewInv_->setUniformUntyped();
	}
	{
		viewProjData_.resize(numLayer_ * 2, Mat4f::identity());
		viewProj_    = std::span<Mat4f>(viewProjData_).subspan(0, numLayer_);
		viewProjInv_ = std::span<Mat4f>(viewProjData_).subspan(numLayer_, numLayer_);
		sh_viewProj_->set_numArrayElements(numLayer_);
		sh_viewProjInv_->set_numArrayElements(numLayer_);
		sh_viewProj_->setUniformUntyped();
		sh_viewProjInv_->setUniformUntyped();
	}

	// Projection is calculated in shaders.
	proj_[0] = Mat4f::identity();
	projInv_[0] = Mat4f::identity();
	sh_proj_->setVertex(0, Mat4f::identity());
	sh_projInv_->setVertex(0, Mat4f::identity());

	projParams_[0] = ProjectionParams(0.1f, 100.0f, 1.0f, 180.0f);
	sh_projParams_->setVertex(0, projParams_[0].asVec4());
	for (unsigned int i = 0; i < numLayer_; ++i) {
		// set frustum parameters
		frustum_[i].setPerspective(1.0f, 180.0f, 0.1f, 100.0f);
	}

	// Initialize directions.
	direction_.resize(numLayer_);
	sh_direction_->set_numArrayElements(numLayer_);
	sh_direction_->setUniformUntyped();
	direction_[0] = Vec4f(0.0, 0.0, 1.0, 0.0);
	sh_direction_->setVertex(0, direction_[0]);
	if (hasBackFace_) {
		direction_[1] = Vec4f(0.0, 0.0, -1.0, 0.0);
		sh_direction_->setVertex(1, direction_[1]);
	}
}

void ParabolicCamera::setNormal(const Vec3f &normal) {
	setDirection(0, -normal);
	if (hasBackFace_) {
		setDirection(1, normal);
	}
}

void ParabolicCamera::updateViewProjection(unsigned int projectionIndex, unsigned int viewIndex) {
	setViewProjection(viewIndex, view(viewIndex));
	setViewProjectionInverse(viewIndex, viewInverse(viewIndex));
	frustum_[viewIndex].update(position(0), direction(viewIndex));
}

LightCamera_Parabolic::LightCamera_Parabolic(const ref_ptr<Light> &light, bool isDualParabolic)
		: ParabolicCamera(isDualParabolic),
		  LightCamera(light,this) {
	v_lightMatrix_.resize(numLayer_, Mat4f::identity());
	sh_lightMatrix_->set_numArrayElements(numLayer_);
	sh_lightMatrix_->set_forceArray(true);
	sh_lightMatrix_->setUniformUntyped();
	setInput(shadowBuffer_);
	updateParabolicLight();
}

bool LightCamera_Parabolic::updateLight() {
	return updateParabolicLight();
}

bool LightCamera_Parabolic::updateParabolicLight() {
	if(updateLightProjection() || updateLightView()) {
		updateViewProjection1();
		// Transforms world space coordinates to homogenous light space
		for (unsigned int i=0; i<v_lightMatrix_.size(); ++i) {
			// note: bias is not applied here, as the projection is done in shaders
			v_lightMatrix_[i] = viewProjection(i);
		}
		camStamp_ += 1;
		return true;
	}
	return false;
}

bool LightCamera_Parabolic::updateLightProjection() {
	const uint32_t radiusStamp = light_->radius()->stampOfReadData();
	if (lightRadiusStamp_ == radiusStamp) { return false; }
	auto radius = light_->radiusStaged(0);
	lightRadiusStamp_ = radiusStamp;
	setPerspective(1.0f, 180.0f, lightNear_, radius.r.y);
	return true;
}

bool LightCamera_Parabolic::updateLightView() {
	const uint32_t posStamp = light_->position()->stampOfReadData();
	const uint32_t dirStamp = light_->direction()->stampOfReadData();
	if (lightPosStamp_ == posStamp && lightDirStamp_ == dirStamp) { return false; }
	auto dir = light_->directionStaged(0);
	lightPosStamp_ = posStamp;
	lightDirStamp_ = dirStamp;
	// Set the position of the light camera
	setPosition(0, light_->positionStaged(0).r.xyz());
	setDirection(0, -dir.r);
	if (hasBackFace_) {
		setDirection(1, dir.r);
	}
	return updateView();
}
