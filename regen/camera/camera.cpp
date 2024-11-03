/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include "camera.h"

using namespace regen;

Camera::Camera(GLboolean initializeMatrices)
		: HasInputState(VBO::USAGE_DYNAMIC),
		  isAudioListener_(GL_FALSE) {
	fov_ = ref_ptr<ShaderInput1f>::alloc("fov");
	fov_->setUniformDataUntyped(nullptr);
	setInput(fov_);

	aspect_ = ref_ptr<ShaderInput1f>::alloc("aspect");
	aspect_->setUniformDataUntyped(nullptr);
	setInput(aspect_);

	near_ = ref_ptr<ShaderInput1f>::alloc("near");
	near_->setUniformDataUntyped(nullptr);
	setInput(near_);

	far_ = ref_ptr<ShaderInput1f>::alloc("far");
	far_->setUniformDataUntyped(nullptr);
	setInput(far_);

	position_ = ref_ptr<ShaderInput3f>::alloc("cameraPosition");
	position_->setUniformData(Vec3f(0.0, 1.0, 4.0));
	setInput(position_);

	direction_ = ref_ptr<ShaderInput3f>::alloc("cameraDirection");
	direction_->setUniformData(Vec3f(0, 0, -1));
	setInput(direction_);

	vel_ = ref_ptr<ShaderInput3f>::alloc("cameraVelocity");
	vel_->setUniformData(Vec3f(0.0f));
	setInput(vel_);

	view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
	setInput(view_);
	viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
	setInput(viewInv_);

	proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
	setInput(proj_);
	projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
	setInput(projInv_);

	viewproj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
	setInput(viewproj_);
	viewprojInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
	setInput(viewprojInv_);

	updateFrustum(8.0 / 6.0, 45.0, 1.0, 200.0, GL_FALSE);
	if (initializeMatrices) {
		view_->setUniformDataUntyped(nullptr);
		viewInv_->setUniformDataUntyped(nullptr);
		proj_->setUniformDataUntyped(nullptr);
		projInv_->setUniformDataUntyped(nullptr);
		viewproj_->setUniformDataUntyped(nullptr);
		viewprojInv_->setUniformDataUntyped(nullptr);

		updateProjection();
		updateLookAt();
		updateViewProjection();
	}
}

void Camera::updateLookAt() {
	view_->setVertex(0, Mat4f::lookAtMatrix(
			position_->getVertex(0),
			direction_->getVertex(0),
			Vec3f::up()));
	viewInv_->setVertex(0, view_->getVertex(0).lookAtInverse());
}

void Camera::updateProjection() {
	proj_->setVertex(0, Mat4f::projectionMatrix(
			fov_->getVertex(0),
			aspect_->getVertex(0),
			near_->getVertex(0),
			far_->getVertex(0))
	);
	projInv_->setVertex(0, proj_->getVertex(0).projectionInverse());
}

void Camera::updateViewProjection(GLuint i, GLuint j) {
	viewproj_->setVertex((i > j ? i : j),
						 view_->getVertex(j) * proj_->getVertex(i));
	viewprojInv_->setVertex((i > j ? i : j),
							projInv_->getVertex(i) * viewInv_->getVertex(j));
}

void Camera::updateFrustum(
		GLfloat aspect,
		GLfloat fov,
		GLfloat near,
		GLfloat far,
		GLboolean updateMatrices) {
	near_->setVertex(0, near);
	far_->setVertex(0, far);
	fov_->setVertex(0, fov);
	aspect_->setVertex(0, aspect);
	frustum_.set(aspect, fov, near, far);

	if (updateMatrices) {
		updateProjection();
		updateViewProjection();
	}
}

void Camera::enable(RenderState *rs) {
	// TODO: do this in animation thread
	frustum_.update(position()->getVertex(0), direction()->getVertex(0));
	HasInputState::enable(rs);
}

GLboolean Camera::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) {
	return frustum_.hasIntersectionWithSphere(center, radius);
}

GLboolean Camera::hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) {
	return frustum_.hasIntersectionWithBox(center, points);
}

void Camera::set_isAudioListener(GLboolean isAudioListener) {
	isAudioListener_ = isAudioListener;
	if (isAudioListener_) {
		AudioListener::set3f(AL_POSITION, position_->getVertex(0));
		AudioListener::set3f(AL_VELOCITY, vel_->getVertex(0));
		AudioListener::set6f(AL_ORIENTATION, Vec6f(direction_->getVertex(0), Vec3f::up()));
	}
}


OmniDirectionalCamera::OmniDirectionalCamera(GLboolean hasBackFace, GLboolean update)
		: Camera(update),
		  hasBackFace_(hasBackFace) {
}

GLboolean OmniDirectionalCamera::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) {
	GLfloat d = Plane(position()->getVertex(0), direction()->getVertex(0)).distance(center);
	if (hasBackFace_) d = abs(d);
	return d - radius < far()->getVertex(0) && d + radius > near()->getVertex(0);
}

GLboolean OmniDirectionalCamera::hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) {
	Plane p(position()->getVertex(0), direction()->getVertex(0));
	for (int i = 0; i < 8; ++i) {
		GLfloat d = p.distance(center + points[i]);
		if (hasBackFace_) d = abs(d);
		if (d > far()->getVertex(0) || d < near()->getVertex(0))
			return GL_FALSE;
	}
	return GL_TRUE;
}
