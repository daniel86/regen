/*
 * reflection-camera.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#include <GL/glew.h>

#include "reflection-camera.h"

using namespace regen;

namespace regen {
	class ReflectionUpdater : public Animation {
	public:
		explicit ReflectionUpdater(ReflectionCamera *camera)
				: Animation(false, true),
				  camera_(camera) {}
		void animate(double dt) override { camera_->updateReflection(); }
	private:
		ReflectionCamera *camera_;
	};
}

ReflectionCamera::ReflectionCamera(
		const ref_ptr<Camera> &userCamera,
		const ref_ptr<Mesh> &mesh,
		unsigned int vertexIndex,
		bool hasBackFace)
		: Camera(1),
		  userCamera_(userCamera),
		  vertexIndex_(vertexIndex),
		  projStamp_(userCamera->projection()->stamp() - 1),
		  camPosStamp_(userCamera->position()->stamp() - 1),
		  camDirStamp_(userCamera->direction()->stamp() - 1),
		  cameraChanged_(GL_TRUE),
		  isFront_(GL_TRUE),
		  hasMesh_(GL_TRUE),
		  hasBackFace_(hasBackFace) {
	setPerspective(
			userCamera_->fov()->getVertex(0).r,
			userCamera_->aspect()->getVertex(0).r,
			userCamera_->near()->getVertex(0).r,
			userCamera_->far()->getVertex(0).r);

	clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
	clipPlane_->setUniformData(Vec4f(0.0f));
	setInput(clipPlane_);

	pos_ = mesh->positions();
	nor_ = mesh->normals();
	isReflectorValid_ = (pos_.get() != nullptr) && (nor_.get() != nullptr);
	if (isReflectorValid_) {
		posStamp_ = pos_->stamp() - 1;
		norStamp_ = nor_->stamp() - 1;
	}

	auto modelMat = mesh->findShaderInput("modelMatrix");
	transform_ = modelMat.value().in;
	if (transform_.get() != nullptr) {
		transformStamp_ = transform_->stamp() - 1;
	}

	reflectionUpdater_ = ref_ptr<ReflectionUpdater>::alloc(this);
	reflectionUpdater_->startAnimation();
}

ReflectionCamera::ReflectionCamera(
		const ref_ptr<Camera> &userCamera,
		const Vec3f &reflectorNormal,
		const Vec3f &reflectorPoint,
		bool hasBackFace)
		: Camera(1),
		  userCamera_(userCamera),
		  projStamp_(userCamera->projection()->stamp() - 1),
		  camPosStamp_(userCamera->position()->stamp() - 1),
		  camDirStamp_(userCamera->direction()->stamp() - 1),
		  cameraChanged_(true),
		  isFront_(true),
		  hasMesh_(false),
		  hasBackFace_(hasBackFace) {
	setPerspective(
			userCamera_->aspect()->getVertex(0).r,
			userCamera_->fov()->getVertex(0).r,
			userCamera_->near()->getVertex(0).r,
			userCamera_->far()->getVertex(0).r);

	clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
	clipPlane_->setUniformData(Vec4f(0.0f));
	setInput(clipPlane_);

	vertexIndex_ = 0;
	transformStamp_ = 0;
	posStamp_ = 0;
	norStamp_ = 0;
	posWorld_ = reflectorPoint;
	norWorld_ = reflectorNormal;
	isReflectorValid_ = true;

	clipPlane_->setVertex(0, Vec4f(
			norWorld_.x, norWorld_.y, norWorld_.z,
			norWorld_.dot(posWorld_)));
	reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_, norWorld_);

	reflectionUpdater_ = ref_ptr<ReflectionUpdater>::alloc(this);
	reflectionUpdater_->startAnimation();
}

void ReflectionCamera::updateReflection() {
	if (isHidden() || !isReflectorValid_) {
		return;
	}

	GLboolean reflectorChanged = GL_FALSE;
	if (hasMesh_) {
		if (transform_.get() != nullptr && transform_->stamp() != transformStamp_) {
			reflectorChanged = GL_TRUE;
			transformStamp_ = transform_->stamp();
		}
		if (nor_->stamp() != norStamp_) {
			reflectorChanged = GL_TRUE;
			norStamp_ = nor_->stamp();
		}
		if (pos_->stamp() != posStamp_) {
			reflectorChanged = GL_TRUE;
			posStamp_ = pos_->stamp();
		}
		// Compute plane parameters...
		if (reflectorChanged) {
			if (!pos_->hasClientData()) pos_->readServerData();
			if (!nor_->hasClientData()) nor_->readServerData();
			posWorld_ = pos_->mapClientData<Vec3f>(ShaderData::READ).r[vertexIndex_];
			norWorld_ = nor_->mapClientData<Vec3f>(ShaderData::READ).r[vertexIndex_];

			if (transform_.get() != nullptr) {
				if (!transform_->hasClientData()) transform_->readServerData();
				auto transform = transform_->mapClientData<Mat4f>(ShaderData::READ);
				posWorld_ = (transform.r[0] ^ Vec4f(posWorld_, 1.0)).xyz_();
				norWorld_ = (transform.r[0] ^ Vec4f(norWorld_, 0.0)).xyz_();
				norWorld_.normalize();
			}
		}
	}

	// Switch normal if viewer is behind reflector.
	GLboolean isFront = norWorld_.dot(
		userCamera_->position()->getVertex(0).r - posWorld_) > 0.0;
	if (isFront != isFront_) {
		isFront_ = isFront;
		reflectorChanged = GL_TRUE;
	}
	// Skip back faces
	if (!isFront && !hasBackFace_) return;

	// Compute reflection matrix...
	if (reflectorChanged) {
		if (isFront_) {
			clipPlane_->setVertex(0, Vec4f(
					norWorld_.x, norWorld_.y, norWorld_.z,
					norWorld_.dot(posWorld_)));
			reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_, norWorld_);
		} else {
			// flip reflector normal
			Vec3f n = -norWorld_;
			clipPlane_->setVertex(0, Vec4f(n.x, n.y, n.z, n.dot(posWorld_)));
			reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_, n);
		}
	}

	// Compute reflection camera direction
	if (reflectorChanged || userCamera_->direction()->stamp() != camDirStamp_) {
		camDirStamp_ = userCamera_->direction()->stamp();
		Vec3f dir = reflectionMatrix_.rotateVector(userCamera_->direction()->getVertex(0).r);
		dir.normalize();
		direction_->setVertex(0, dir);

		reflectorChanged = GL_TRUE;
	}
	// Compute reflection camera position
	if (reflectorChanged || userCamera_->position()->stamp() != camPosStamp_) {
		camPosStamp_ = userCamera_->position()->stamp();
		Vec3f reflected = reflectionMatrix_.transformVector(
			userCamera_->position()->getVertex(0).r);
		position_->setVertex(0, reflected);

		reflectorChanged = GL_TRUE;
	}

	// Compute view matrix
	if (reflectorChanged) {
		updateView();
		cameraChanged_ = GL_TRUE;
	}

	// Compute projection matrix
	if (userCamera_->projection()->stamp() != projStamp_) {
		projStamp_ = userCamera_->projection()->stamp();
		proj_->setUniformData(
				userCamera_->projection()->getVertex(0).r);
		projInv_->setUniformData(
				userCamera_->projectionInverse()->getVertex(0).r);
		cameraChanged_ = GL_TRUE;
	}

	// Compute view-projection matrix
	if (cameraChanged_) {
		updateViewProjection(0u,0u);
		cameraChanged_ = GL_FALSE;
	}
}
