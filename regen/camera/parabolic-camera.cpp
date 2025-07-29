#include "parabolic-camera.h"

using namespace regen;

ParabolicCamera::ParabolicCamera(bool isDualParabolic)
		: Camera((isDualParabolic ? 2 : 1)),
		  hasBackFace_(isDualParabolic) {
	shaderDefine("RENDER_TARGET", isDualParabolic ? "DUAL_PARABOLOID" : "PARABOLOID");
	shaderDefine("USE_PARABOLOID_PROJECTION", "TRUE");
	isOmni_ = true;

	// Set vector size
	view_.resize(numLayer_, Mat4f::identity());
	viewInv_.resize(numLayer_, Mat4f::identity());
	viewProj_.resize(numLayer_, Mat4f::identity());
	viewProjInv_.resize(numLayer_, Mat4f::identity());
	direction_.resize(numLayer_);

	// Set matrix array size
	sh_view_->set_numArrayElements(numLayer_);
	sh_viewInv_->set_numArrayElements(numLayer_);
	sh_viewProj_->set_numArrayElements(numLayer_);
	sh_viewProjInv_->set_numArrayElements(numLayer_);

	// Allocate matrices
	sh_view_->setUniformUntyped();
	sh_viewInv_->setUniformUntyped();
	sh_viewProj_->setUniformUntyped();
	sh_viewProjInv_->setUniformUntyped();

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
