/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include "camera.h"

using namespace regen;

namespace regen {
	class CameraMotion : public Animation {
	public:
		explicit CameraMotion(Camera *camera)
				: Animation(true, false),
				  camera_(camera) {}
		// TODO: rather use CPU thread for computation, but it needs synchronization
		//void animate(double dt) override { camera_->updatePose(); }
		void glAnimate(RenderState *rs, double dt) override { camera_->updatePose(); }
	private:
		Camera *camera_;
	};
}

Camera::Camera(unsigned int numLayer)
		: HasInputState(VBO::USAGE_DYNAMIC),
		  numLayer_(numLayer),
		  frustum_(numLayer) {
	// add shader constants via defines
	shaderDefine("RENDER_LAYER", REGEN_STRING(numLayer_));

	fov_ = ref_ptr<ShaderInput1f>::alloc("fov");
	fov_->setUniformData(60.0f);

	aspect_ = ref_ptr<ShaderInput1f>::alloc("aspect");
	aspect_->setUniformData(8.0f / 6.0f);

	near_ = ref_ptr<ShaderInput1f>::alloc("near");
	near_->setUniformData(0.1f);

	far_ = ref_ptr<ShaderInput1f>::alloc("far");
	far_->setUniformData(100.0f);

	position_ = ref_ptr<ShaderInput3f>::alloc("cameraPosition");
	position_->setUniformData(Vec3f(0.0, 1.0, 4.0));

	direction_ = ref_ptr<ShaderInput3f>::alloc("cameraDirection");
	direction_->setUniformData(Vec3f(0, 0, -1));

	vel_ = ref_ptr<ShaderInput3f>::alloc("cameraVelocity");
	vel_->setUniformData(Vec3f(0.0f));

	view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
	view_->setUniformData(Mat4f::identity());
	viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
	viewInv_->setUniformData(Mat4f::identity());

	proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
	proj_->setUniformData(Mat4f::identity());
	projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
	projInv_->setUniformData(Mat4f::identity());

	viewProj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
	viewProj_->setUniformData(Mat4f::identity());
	viewProjInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
	viewProjInv_->setUniformData(Mat4f::identity());

	cameraBlock_ = ref_ptr<UniformBlock>::alloc("Camera");
	cameraBlock_->addUniform(view_);
	cameraBlock_->addUniform(viewInv_);
	cameraBlock_->addUniform(proj_);
	cameraBlock_->addUniform(projInv_);
	cameraBlock_->addUniform(viewProj_);
	cameraBlock_->addUniform(viewProjInv_);
	cameraBlock_->addUniform(position_);
	cameraBlock_->addUniform(fov_);
	cameraBlock_->addUniform(direction_);
	cameraBlock_->addUniform(aspect_);
	cameraBlock_->addUniform(vel_);
	cameraBlock_->addUniform(near_);
	cameraBlock_->addUniform(far_);
	setInput(cameraBlock_);
}

void Camera::setPerspective(float aspect, float fov, float near, float far) {
	bool hasLayeredProjection = proj_->elementCount() > 1;
	if (hasLayeredProjection) {
		for (unsigned int i = 0; i < numLayer_; ++i) {
			setPerspective(aspect, fov, near, far, i);
		}
	} else {
		setPerspective(aspect, fov, near, far, 0);
		for (unsigned int i = 1; i < numLayer_; ++i) {
			frustum_[i].setPerspective(aspect, fov, near, far);
		}
	}
}

void Camera::setPerspective(float aspect, float fov, float near, float far, unsigned int layer) {
	fov_->setVertexOrFirst(layer, fov);
	aspect_->setVertexOrFirst(layer, aspect);
	near_->setVertexOrFirst(layer, near);
	far_->setVertexOrFirst(layer, far);
	frustum_[layer].setPerspective(aspect, fov, near, far);

	auto projectionMatrix = Mat4f::projectionMatrix(fov, aspect, near, far);
	proj_->setVertexOrFirst(layer, projectionMatrix);
	projInv_->setVertexOrFirst(layer, projectionMatrix.projectionInverse());
	isOrtho_ = false;
}

void Camera::setOrtho(float left, float right, float bottom, float top, float near, float far) {
	for (unsigned int i = 0; i < numLayer_; ++i) {
		setOrtho(left, right, bottom, top, near, far, i);
	}
}

void Camera::setOrtho(float left, float right, float bottom, float top, float near, float far, unsigned int layer) {
	fov_->setVertex(0, 0.0f);
	aspect_->setVertex(0, 1.0f);
	near_->setVertexOrFirst(layer, near);
	far_->setVertexOrFirst(layer, far);
	frustum_[layer].setOrtho(left, right, bottom, top, near, far);
	proj_->setVertex(layer, Mat4f::orthogonalMatrix(left, right, bottom, top, near, far));
	projInv_->setVertex(layer, proj_->getVertex(layer).orthogonalInverse());
	isOrtho_ = true;
}

bool Camera::updateCamera() {
	auto projectionStamp = proj_->stamp();
	if (updateView() || projectionStamp != projectionStamp_) {
		updateViewProjection1();
		projectionStamp_ = projectionStamp;
		return true;
	} else {
		return false;
	}
}

bool Camera::updateView() {
	auto posStamp = position_->stamp();
	auto dirStamp = direction_->stamp();
	if (posStamp == posStamp_ && dirStamp == dirStamp_) { return false; }
	posStamp_ = posStamp;
	dirStamp_ = dirStamp;

	auto numViewLayers = view_->elementCount();
	for (unsigned int i = 0; i < numViewLayers; ++i) {
		auto &dir = direction_->getVertexOrFirst(i);
		if (std::abs(dir.dot(Vec3f::up())) > 0.999f) {
			auto viewMatrix = Mat4f::lookAtMatrix(
				position_->getVertexOrFirst(i),
				dir, Vec3f::right());
			view_->setVertex(i, viewMatrix);
			viewInv_->setVertex(i, viewMatrix.lookAtInverse());
		} else {
			auto viewMatrix = Mat4f::lookAtMatrix(
				position_->getVertexOrFirst(i),
				dir, Vec3f::up());
			view_->setVertex(i, viewMatrix);
			viewInv_->setVertex(i, viewMatrix.lookAtInverse());
		}
	}

	return true;
}

void Camera::updateViewProjection1() {
	auto numViewLayers = view_->elementCount();
	auto numProjLayers = proj_->elementCount();
	auto maxIndex = std::max(numViewLayers, numProjLayers);
	for (unsigned int i = 0; i < maxIndex; ++i) {
		updateViewProjection(
			numProjLayers > 1 ? i : 0,
			numViewLayers > 1 ? i : 0);
	}
}

void Camera::updateViewProjection(unsigned int projectionIndex, unsigned int viewIndex) {
	auto maxIndex = std::max(projectionIndex, viewIndex);
	viewProj_->setVertex(maxIndex,
		view_->getVertex(viewIndex) * proj_->getVertex(projectionIndex));
	viewProjInv_->setVertex(maxIndex,
		projInv_->getVertex(projectionIndex) * viewInv_->getVertex(viewIndex));
	frustum_[maxIndex].update(
		position()->getVertexOrFirst(maxIndex),
		direction()->getVertexOrFirst(maxIndex));
}

void Camera::set_isAudioListener(GLboolean isAudioListener) {
	isAudioListener_ = isAudioListener;
	if (isAudioListener_) {
		AudioListener::set3f(AL_POSITION, position_->getVertex(0));
		AudioListener::set3f(AL_VELOCITY, vel_->getVertex(0));
		AudioListener::set6f(AL_ORIENTATION, Vec6f(direction_->getVertex(0), Vec3f::up()));
	}
}

void Camera::updatePose() {
	bool updated = false;
	if (attachedPosition_.get()) {
		if (poseStamp_ != attachedPosition_->stamp()) {
			poseStamp_ = attachedPosition_->stamp();
			position_->setUniformData(attachedPosition_->getVertex(0));
			updated = true;
		}
	}
	else if (attachedTransform_.get()) {
		if (poseStamp_ != attachedTransform_->stamp()) {
			poseStamp_ = attachedTransform_->stamp();
			auto &m = attachedTransform_->getVertex(0);
			position_->setVertex(0, m.position());
			if (!isAttachedToPosition_) {
				// TODO: change camera orientation based on transform
				//direction_->setVertex(0, (m ^ Vec4f(Vec3f::front(),0.0)).xyz_());
			}
			updated = true;
		}
	}

	if (updated) {
		updateCamera();
	}
}

void Camera::attachToPosition(const ref_ptr<ShaderInput3f> &attachedPosition) {
	attachedPosition_ = attachedPosition;
	attachedTransform_ = {};
	poseStamp_ = 0;
	if (!attachedMotion_.get()) {
		attachedMotion_ = ref_ptr<CameraMotion>::alloc(this);
	}
}

void Camera::attachToPosition(const ref_ptr<ShaderInputMat4> &attachedTransform) {
	attachedPosition_ = {};
	attachedTransform_ = attachedTransform;
	poseStamp_ = 0;
	isAttachedToPosition_ = true;
	if (!attachedMotion_.get()) {
		attachedMotion_ = ref_ptr<CameraMotion>::alloc(this);
	}
}

void Camera::attachToTransform(const ref_ptr<ShaderInputMat4> &attachedTransform) {
	attachedPosition_ = {};
	attachedTransform_ = attachedTransform;
	poseStamp_ = 0;
	isAttachedToPosition_ = false;
	if (!attachedMotion_.get()) {
		attachedMotion_ = ref_ptr<CameraMotion>::alloc(this);
	}
}

bool Camera::hasSphereIntersection(const Vec3f &center, GLfloat radius) const {
	auto d = Plane(
		position()->getVertex(0),
		direction()->getVertex(0)).distance(center);
	return d - radius < far()->getVertex(0) &&
	       d + radius > near()->getVertex(0);
}

bool Camera::hasSphereIntersection(const Vec3f &center, const Vec3f *points) const {
	Plane p(position()->getVertex(0), direction()->getVertex(0));
	for (int i = 0; i < 8; ++i) {
		auto d = p.distance(center + points[i]);
		if (d > far()->getVertex(0) || d < near()->getVertex(0))
			return false;
	}
	return true;
}

bool Camera::hasHalfSphereIntersection(const Vec3f &center, GLfloat radius) const {
	// get the distance from the camera to the center of the sphere
	auto d = Plane(
		position()->getVertex(0),
		direction()->getVertex(0)).distance(center);
	// check if the sphere is outside the far plane
	if (d - radius > far()->getVertex(0)) return false;
	// check if the sphere is inside the near plane
	if (d + radius < near()->getVertex(0)) return false;
	// check if the sphere is inside the half sphere
	auto halfSphereRadius = far()->getVertex(0);
	auto halfSphereNormal = direction()->getVertex(0);
	auto halfSphereCenter = position()->getVertex(0) + halfSphereNormal * halfSphereRadius;
	return Plane(halfSphereCenter, halfSphereNormal).distance(center) < radius;
}

bool Camera::hasHalfSphereIntersection(const Vec3f &center, const Vec3f *points) const {
	// get the distance from the camera to the center of the sphere
	auto d = Plane(
		position()->getVertex(0),
		direction()->getVertex(0)).distance(center);
	// check if the sphere is outside the far plane
	if (d > far()->getVertex(0)) return false;
	// check if the sphere is inside the near plane
	if (d < near()->getVertex(0)) return false;
	// check if the sphere is inside the half sphere
	auto halfSphereRadius = far()->getVertex(0);
	auto halfSphereNormal = direction()->getVertex(0);
	auto halfSphereCenter = position()->getVertex(0) + halfSphereNormal * halfSphereRadius;
	auto halfSphere = Plane(halfSphereCenter, halfSphereNormal);
	for (int i = 0; i < 8; ++i) {
		if (halfSphere.distance(center + points[i]) < 0) {
			return false;
		}
	}
	return true;
}

bool Camera::hasFrustumIntersection(const Vec3f &center, GLfloat radius) const {
	for (auto &f : frustum_) {
		if (f.hasIntersectionWithSphere(center, radius)) {
			return true;
		}
	}
	return false;
}

bool Camera::hasFrustumIntersection(const Vec3f &center, const Vec3f *points) const {
	for (auto &f : frustum_) {
		if (f.hasIntersectionWithBox(center, points)) {
			return true;
		}
	}
	return false;
}

bool Camera::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) const {
	if (isOmni_) {
		return hasSphereIntersection(center, radius);
	}
	//else if (isSemiOmni_) {
	//	return hasHalfSphereIntersection(center, radius);
	//}
	else {
		return hasFrustumIntersection(center, radius);
	}
}

bool Camera::hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) const {
	if (isOmni_) {
		return hasSphereIntersection(center, points);
	}
	//else if (isSemiOmni_) {
	//	return hasHalfSphereIntersection(center, points);
	//}
	else {
		return hasFrustumIntersection(center, points);
	}
}

