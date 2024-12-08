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
		  isAudioListener_(GL_FALSE),
		  camStamp_(0) {

	fov_ = ref_ptr<ShaderInput1f>::alloc("fov");
	fov_->setUniformDataUntyped(nullptr);

	aspect_ = ref_ptr<ShaderInput1f>::alloc("aspect");
	aspect_->setUniformDataUntyped(nullptr);

	near_ = ref_ptr<ShaderInput1f>::alloc("near");
	near_->setUniformDataUntyped(nullptr);

	far_ = ref_ptr<ShaderInput1f>::alloc("far");
	far_->setUniformDataUntyped(nullptr);

	position_ = ref_ptr<ShaderInput3f>::alloc("cameraPosition");
	position_->setUniformData(Vec3f(0.0, 1.0, 4.0));

	direction_ = ref_ptr<ShaderInput3f>::alloc("cameraDirection");
	direction_->setUniformData(Vec3f(0, 0, -1));

	vel_ = ref_ptr<ShaderInput3f>::alloc("cameraVelocity");
	vel_->setUniformData(Vec3f(0.0f));

	view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
	viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");

	proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
	projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");

	viewproj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
	viewprojInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");

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

#if 0
	cameraBlock_ = ref_ptr<UniformBlock>::alloc("Camera");
	cameraBlock_->addUniform(fov_);
	cameraBlock_->addUniform(aspect_);
	cameraBlock_->addUniform(near_);
	cameraBlock_->addUniform(far_);
	cameraBlock_->addUniform(position_);
	cameraBlock_->addUniform(direction_);
	cameraBlock_->addUniform(vel_);
	cameraBlock_->addUniform(view_);
	cameraBlock_->addUniform(viewInv_);
	cameraBlock_->addUniform(proj_);
	cameraBlock_->addUniform(projInv_);
	cameraBlock_->addUniform(viewproj_);
	cameraBlock_->addUniform(viewprojInv_);
	setInput(cameraBlock_);
#else
	setInput(fov_);
	setInput(aspect_);
	setInput(near_);
	setInput(far_);
	setInput(position_);
	setInput(direction_);
	setInput(vel_);
	setInput(view_);
	setInput(viewInv_);
	setInput(proj_);
	setInput(projInv_);
	setInput(viewproj_);
	setInput(viewprojInv_);
#endif
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
	auto posStamp = position_->stamp();
	auto dirStamp = direction_->stamp();
	if (posStamp != posStamp_ || dirStamp != dirStamp_) {
		posStamp_ = posStamp;
		dirStamp_ = dirStamp;
		frustum_.update(position()->getVertex(0), direction()->getVertex(0));
		camStamp_ += 1;
	}
	HasInputState::enable(rs);
}

GLboolean Camera::hasSpotIntersectionWithSphere(const Vec3f &center, GLfloat radius) {
	return frustum_.hasIntersectionWithSphere(center, radius);
}

GLboolean Camera::hasSpotIntersectionWithBox(const Vec3f &center, const Vec3f *points) {
	return frustum_.hasIntersectionWithBox(center, points);
}

GLboolean Camera::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) {
	return hasSpotIntersectionWithSphere(center, radius);
}

GLboolean Camera::hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) {
	return hasSpotIntersectionWithBox(center, points);
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

GLboolean OmniDirectionalCamera::hasOmniIntersectionWithSphere(const Vec3f &center, GLfloat radius) {
	GLfloat d = Plane(position()->getVertex(0), direction()->getVertex(0)).distance(center);
	if (hasBackFace_) d = abs(d);
	return d - radius < far()->getVertex(0) && d + radius > near()->getVertex(0);
}

GLboolean OmniDirectionalCamera::hasOmniIntersectionWithBox(const Vec3f &center, const Vec3f *points) {
	Plane p(position()->getVertex(0), direction()->getVertex(0));
	for (int i = 0; i < 8; ++i) {
		GLfloat d = p.distance(center + points[i]);
		if (hasBackFace_) d = abs(d);
		if (d > far()->getVertex(0) || d < near()->getVertex(0))
			return GL_FALSE;
	}
	return GL_TRUE;
}

GLboolean OmniDirectionalCamera::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) {
	return hasOmniIntersectionWithSphere(center, radius);
}

GLboolean OmniDirectionalCamera::hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) {
	return hasOmniIntersectionWithBox(center, points);
}
