#include "reflection-camera.h"

using namespace regen;

namespace regen {
	class ReflectionUpdater : public Animation {
	public:
		explicit ReflectionUpdater(ReflectionCamera *camera)
				: Animation(false, true),
				  camera_(camera) {}
		void animate(double dt) override {
			if(camera_->updateReflection()) {
				camera_->updateShaderData(static_cast<float>(dt));
			}
		}
	private:
		ReflectionCamera *camera_;
	};
}

ReflectionCamera::ReflectionCamera(
		const ref_ptr<Camera> &userCamera,
		const ref_ptr<Mesh> &mesh,
		uint32_t vertexIndex,
		bool hasBackFace)
		: Camera(1),
		  userCamera_(userCamera),
		  vertexIndex_(vertexIndex),
		  projStamp_(userCamera->projectionStamp() - 1),
		  camPosStamp_(userCamera->positionStamp() - 1),
		  camDirStamp_(userCamera->directionStamp() - 1),
		  cameraChanged_(true),
		  isFront_(true),
		  hasMesh_(true),
		  hasBackFace_(hasBackFace) {
	setPerspective(userCamera_->projParams()[0]);

	pos_ = mesh->positions();
	nor_ = mesh->normals();
	isReflectorValid_ = (pos_.get() != nullptr) && (nor_.get() != nullptr);
	if (isReflectorValid_) {
		posStamp_ = pos_->stampOfReadData() - 1;
		norStamp_ = nor_->stampOfReadData() - 1;
	}

	auto modelMat = mesh->findShaderInput("modelMatrix");
	transform_ = modelMat.value().in;
	if (transform_.get() != nullptr) {
		transformStamp_ = transform_->stampOfReadData() - 1;
	}

	sh_clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
	sh_clipPlane_->setUniformData(Vec4f::zero());
	cameraBlock_->addStagedInput(sh_clipPlane_);

	reflectionUpdater_ = ref_ptr<ReflectionUpdater>::alloc(this);
	reflectionUpdater_->startAnimation();
}

ReflectionCamera::ReflectionCamera(
		const ref_ptr<Camera> &userCamera,
		const Vec3f &reflectorNormal,
		const Vec3f &reflectorPoint,
		bool hasBackFace)
		: Camera(1, userCamera->cameraBlock()->stagingUpdateHint()),
		  userCamera_(userCamera),
		  projStamp_(userCamera->projectionStamp() - 1),
		  camPosStamp_(userCamera->positionStamp() - 1),
		  camDirStamp_(userCamera->directionStamp() - 1),
		  cameraChanged_(true),
		  isFront_(true),
		  hasMesh_(false),
		  hasBackFace_(hasBackFace) {
	setPerspective(userCamera_->projParams()[0]);

	vertexIndex_ = 0;
	transformStamp_ = 0;
	posStamp_ = 0;
	norStamp_ = 0;
	posWorld_ = reflectorPoint;
	norWorld_ = reflectorNormal;
	isReflectorValid_ = true;

	reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_, norWorld_);

	clipPlane_[0] = Vec4f(
			norWorld_.x, norWorld_.y, norWorld_.z,
			norWorld_.dot(posWorld_));
	sh_clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
	sh_clipPlane_->setUniformData(clipPlane_[0]);
	cameraBlock_->addStagedInput(sh_clipPlane_);

	reflectionUpdater_ = ref_ptr<ReflectionUpdater>::alloc(this);
	reflectionUpdater_->startAnimation();
}

bool ReflectionCamera::updateReflection() {
	if (isHidden() || !isReflectorValid_) {
		return false;
	}

	bool reflectorChanged = false;
	if (hasMesh_) {
		if (transform_.get() != nullptr && transform_->stampOfReadData() != transformStamp_) {
			reflectorChanged = true;
			transformStamp_ = transform_->stampOfReadData();
		}
		if (nor_->stampOfReadData() != norStamp_) {
			reflectorChanged = true;
			norStamp_ = nor_->stampOfReadData();
		}
		if (pos_->stampOfReadData() != posStamp_) {
			reflectorChanged = true;
			posStamp_ = pos_->stampOfReadData();
		}
		// Compute plane parameters...
		if (reflectorChanged) {
			if (!pos_->hasClientData()) pos_->readServerData();
			if (!nor_->hasClientData()) nor_->readServerData();
			posWorld_ = pos_->mapClientData<Vec3f>(BUFFER_GPU_READ).r[vertexIndex_];
			norWorld_ = nor_->mapClientData<Vec3f>(BUFFER_GPU_READ).r[vertexIndex_];

			if (transform_.get() != nullptr) {
				if (!transform_->hasClientData()) transform_->readServerData();
				auto transform = transform_->mapClientData<Mat4f>(BUFFER_GPU_READ);
				posWorld_ = (transform.r[0] ^ Vec4f::create(posWorld_, 1.0)).xyz();
				norWorld_ = (transform.r[0] ^ Vec4f::create(norWorld_, 0.0)).xyz();
				norWorld_.normalize();
			}
		}
	}

	// Switch normal if viewer is behind reflector.
	bool isFront = norWorld_.dot(userCamera_->position(0) - posWorld_) > 0.0;
	if (isFront != isFront_) {
		isFront_ = isFront;
		reflectorChanged = true;
	}
	// Skip back faces
	if (!isFront && !hasBackFace_) return false;

	// Compute reflection matrix...
	if (reflectorChanged) {
		if (isFront_) {
			setClipPlane(0, Vec4f(
					norWorld_.x, norWorld_.y, norWorld_.z,
					norWorld_.dot(posWorld_)));
			reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_, norWorld_);
		} else {
			// flip reflector normal
			Vec3f n = -norWorld_;
			setClipPlane(0, Vec4f(n.x, n.y, n.z, n.dot(posWorld_)));
			reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_, n);
		}
	}

	// Compute reflection camera direction
	if (reflectorChanged || userCamera_->directionStamp() != camDirStamp_) {
		camDirStamp_ = userCamera_->directionStamp();
		Vec3f dir = reflectionMatrix_.rotateVector(userCamera_->direction(0));
		dir.normalize();
		setDirection(0, dir);

		reflectorChanged = true;
	}
	// Compute reflection camera position
	if (reflectorChanged || userCamera_->positionStamp() != camPosStamp_) {
		camPosStamp_ = userCamera_->positionStamp();
		setPosition(0,  reflectionMatrix_.transformVector(userCamera_->position(0)));

		reflectorChanged = true;
	}

	// Compute view matrix
	if (reflectorChanged) {
		updateView();
		cameraChanged_ = true;
	}

	// Compute projection matrix
	if (userCamera_->projectionStamp() != projStamp_) {
		projStamp_ = userCamera_->projectionStamp();
		setProjection(0, userCamera_->projection(0));
		setProjectionInverse(0, userCamera_->projectionInverse(0));
		cameraChanged_ = true;
	}

	// Compute view-projection matrix
	if (cameraChanged_) {
		updateViewProjection(0u,0u);
		cameraChanged_ = false;
		camStamp_ += 1u;
		return true;
	} else {
		return false;
	}
}
