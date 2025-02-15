#include "parabolic-camera.h"

using namespace regen;

ParabolicCamera::ParabolicCamera(bool isDualParabolic)
		: Camera((isDualParabolic ? 2 : 1)),
		  hasBackFace_(isDualParabolic) {
	shaderDefine("RENDER_TARGET", isDualParabolic ? "DUAL_PARABOLOID" : "PARABOLOID");
	shaderDefine("USE_PARABOLOID_PROJECTION", "TRUE");
	isOmni_ = true;

	// Set matrix array size
	view_->set_numArrayElements(numLayer_);
	viewInv_->set_numArrayElements(numLayer_);
	viewProj_->set_numArrayElements(numLayer_);
	viewProjInv_->set_numArrayElements(numLayer_);

	// Allocate matrices
	view_->setUniformUntyped();
	viewInv_->setUniformUntyped();
	viewProj_->setUniformUntyped();
	viewProjInv_->setUniformUntyped();

	// Projection is calculated in shaders.
	proj_->setVertex(0, Mat4f::identity());
	projInv_->setVertex(0, Mat4f::identity());
	aspect_->setVertex(0, 1.0f);
	fov_->setVertex(0, 180.0f);
	near_->setVertex(0, 0.1f);
	far_->setVertex(0, 100.0f);
	for (unsigned int i = 0; i < numLayer_; ++i) {
		// set frustum parameters
		frustum_[i].setPerspective(1.0f, 180.0f, 0.1f, 100.0f);
	}

	// Initialize directions.
	direction_->set_numArrayElements(numLayer_);
	direction_->setUniformUntyped();
	direction_->setVertex(0, Vec3f(0.0, 0.0, 1.0));
	if (hasBackFace_) {
		direction_->setVertex(1, Vec3f(0.0, 0.0, -1.0));
	}
}

void ParabolicCamera::setNormal(const Vec3f &normal) {
	direction_->setVertex(0, -normal);
	if (hasBackFace_) direction_->setVertex(1, normal);
}

void ParabolicCamera::updateViewProjection(unsigned int projectionIndex, unsigned int viewIndex) {
	viewProj_->setVertex(viewIndex, view_->getVertex(viewIndex).r);
	viewProjInv_->setVertex(viewIndex, viewInv_->getVertex(viewIndex).r);
	frustum_[viewIndex].update(
		position()->getVertex(0).r,
		direction()->getVertex(viewIndex).r);
}
