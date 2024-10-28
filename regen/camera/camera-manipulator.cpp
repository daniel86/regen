/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include <regen/av/audio.h>

#include "camera-manipulator.h"

using namespace regen;

CameraUpdater::CameraUpdater(const ref_ptr<Camera> &cam)
		: cam_(cam) {
}

void CameraUpdater::computeMatrices(const Vec3f &pos, const Vec3f &dir) {
	Mat4f &proj = *(Mat4f *) cam_->projection()->ownedClientData();
	Mat4f &projInv = *(Mat4f *) cam_->projectionInverse()->ownedClientData();
	view_ = Mat4f::lookAtMatrix(pos, dir, Vec3f::up());
	viewInv_ = view_.lookAtInverse();
	viewproj_ = view_ * proj;
	viewprojInv_ = projInv * viewInv_;
}

void CameraUpdater::updateCamera(const Vec3f &pos, const Vec3f &dir, GLdouble dt) {
	cam_->position()->setVertex(0, pos);
	cam_->direction()->setVertex(0, dir);

	velocity_ = (lastPosition_ - pos) / dt;
	cam_->velocity()->setVertex(0, velocity_);
	lastPosition_ = pos;

	cam_->view()->setVertex(0, view_);
	cam_->viewInverse()->setVertex(0, viewInv_);
	cam_->viewProjection()->setVertex(0, viewproj_);
	cam_->viewProjectionInverse()->setVertex(0, viewprojInv_);

	if (cam_->isAudioListener()) {
		AudioListener::set3f(AL_POSITION, pos);
		AudioListener::set3f(AL_VELOCITY, velocity_);
		AudioListener::set6f(AL_ORIENTATION, Vec6f(dir, Vec3f::up()));
	}
}

////////////////
////////////////
////////////////

KeyFrameCameraTransform::KeyFrameCameraTransform(const ref_ptr<Camera> &cam)
		: Animation(GL_TRUE, GL_TRUE),
		  CameraUpdater(cam),
		  repeat_(GL_TRUE),
		  skipFirstFrameOnLoop_(GL_TRUE),
		  pauseTime_(0.0),
		  currentPauseDuration_(0.0),
		  isPaused_(GL_FALSE) {
	camPos_ = cam->position()->getVertex(0);
	camDir_ = cam->direction()->getVertex(0);
	it_ = frames_.end();
	lastFrame_.pos = camPos_;
	lastFrame_.dir = camDir_;
	lastFrame_.dt = 0.0;
	dt_ = 0.0;
	easeInOutIntensity_ = 1.0;
	computeMatrices(camPos_, camDir_);
	updateCamera(camPos_, camDir_, 0.0);
}

void KeyFrameCameraTransform::push_back(const Vec3f &pos, const Vec3f &dir, GLdouble dt) {
	CameraKeyFrame f;
	f.pos = pos;
	f.dir = dir;
	f.dt = dt;
	frames_.push_back(f);
	if (frames_.size() == 1) {
		it_ = frames_.begin();
		lastFrame_ = *it_;
	}
}

static inline auto easeInOutCubic(GLdouble t, GLdouble intensity) {
	t = pow(t, intensity);
	return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

Vec3f KeyFrameCameraTransform::interpolatePosition(const Vec3f &v0, const Vec3f &v1, GLdouble t) const {
	GLdouble sample;
	if (easeInOutIntensity_ > 0.0) {
		sample = easeInOutCubic(t, easeInOutIntensity_);
	} else {
		sample = t;
	}
    return math::mix(v0, v1, sample);
}

Vec3f KeyFrameCameraTransform::interpolateDirection(const Vec3f &v0, const Vec3f &v1, GLdouble t) const {
	GLdouble sample;
	if (easeInOutIntensity_ > 0.0) {
		sample = easeInOutCubic(t, easeInOutIntensity_);
	} else {
		sample = t;
	}
    return math::slerp(v0, v1, sample);
}

void KeyFrameCameraTransform::animate(GLdouble dt) {
	if (it_ == frames_.end()) return; // end reached
	CameraKeyFrame &currentFrame = *it_;
	GLdouble dtSeconds = dt / 1000.0;

	// handle state where we wait between frames.
	// dt_ will not be changed in the meantime.
	if (isPaused_) {
		currentPauseDuration_ += dtSeconds;
		if(currentPauseDuration_ >= pauseTime_) {
			isPaused_ = GL_FALSE;
			dtSeconds = currentPauseDuration_ - pauseTime_;
			currentPauseDuration_ = 0.0;
		} else {
			return;
		}
	}
	// below we are not in pause mode
	dt_ += dtSeconds;

	// reached next frame?
	if (dt_ >= currentFrame.dt) {
		GLdouble dtNewFrame = dt_ - currentFrame.dt;
		// move to next frame
		++it_;
		lastFrame_ = currentFrame;
		// loop back to first frame
		if (it_ == frames_.end()) {
			if (repeat_) {
				it_ = frames_.begin();
				if (skipFirstFrameOnLoop_) {
					++it_;
					if (it_ == frames_.end()) return;
				}
			} else {
				stopAnimation();
				return;
			}
		}
		// enter pause mode if pause time is set
		if (pauseTime_ > 0.0) {
			isPaused_ = GL_TRUE;
			currentPauseDuration_ = dtNewFrame;
			dt_ = 0.0;
			return;
		} else {
			dt_ = 0.0;
			animate(dtNewFrame);
			return;
		}
	}

	Vec3f &pos0 = lastFrame_.pos;
	Vec3f &pos1 = currentFrame.pos;
	Vec3f dir0 = lastFrame_.dir;
	Vec3f dir1 = currentFrame.dir;
	GLdouble t = currentFrame.dt > 0.0 ? dt_ / currentFrame.dt : 1.0;
	dir0.normalize();
	dir1.normalize();

	lock();
	{
		camPos_ = interpolatePosition(pos0, pos1, t);
		camDir_ = interpolateDirection(dir0, dir1, t);
		camDir_.normalize();
		computeMatrices(camPos_, camDir_);
	}
	unlock();
}

void KeyFrameCameraTransform::glAnimate(RenderState *rs, GLdouble dt) {
	lock();
	{
		updateCamera(camPos_, camDir_, dt);
	}
	unlock();
}

////////////////
////////////////
////////////////

FirstPersonTransform::FirstPersonTransform(const ref_ptr<ShaderInputMat4> &mat, const ref_ptr<Mesh> &mesh)
		: Animation(GL_TRUE, GL_TRUE),
		  mat_(mat),
		  mesh_(mesh),
		  physicsMode_(IMPULSE),
		  physicsSpeedFactor_(80.0) {
	horizontalOrientation_ = 0.0;
	moveAmount_ = 1.0;
	moveForward_ = GL_FALSE;
	moveBackward_ = GL_FALSE;
	moveLeft_ = GL_FALSE;
	moveRight_ = GL_FALSE;
	isMoving_ = GL_FALSE;
	matVal_ = Mat4f::identity();

	auto hasPhysics = (mesh_.get() && !mesh_->physicalObjects().empty());
	if (hasPhysics) {
		for (const auto &po: mesh_->physicalObjects()) {
			auto &motionState = po->motionState();
			btTransform transform;
			motionState->getWorldTransform(transform);
			transform.getOpenGLMatrix((btScalar *) matVal_.x);
			auto rigidBody = po->rigidBody();
			rigidBody->setLinearVelocity(btVector3(0, 0, 0));
		}
	}
}

void FirstPersonTransform::set_moveAmount(GLfloat moveAmount) {
	moveAmount_ = moveAmount;
}

void FirstPersonTransform::moveForward(GLboolean v) {
	moveForward_ = v;
}

void FirstPersonTransform::moveBackward(GLboolean v) {
	moveBackward_ = v;
}

void FirstPersonTransform::moveLeft(GLboolean v) {
	moveLeft_ = v;
}

void FirstPersonTransform::moveRight(GLboolean v) {
	moveRight_ = v;
}

void FirstPersonTransform::stepForward(const GLfloat &v) { step(dirXZ_ * v); }

void FirstPersonTransform::stepBackward(const GLfloat &v) { step(dirXZ_ * (-v)); }

void FirstPersonTransform::stepLeft(const GLfloat &v) { step(dirSidestep_ * (-v)); }

void FirstPersonTransform::stepRight(const GLfloat &v) { step(dirSidestep_ * v); }

void FirstPersonTransform::step(const Vec3f &v) { step_ += v; }

void FirstPersonTransform::lookLeft(GLdouble amount) {
	horizontalOrientation_ = fmod(horizontalOrientation_ + amount, 2.0 * M_PI);
}

void FirstPersonTransform::lookRight(GLdouble amount) {
	horizontalOrientation_ = fmod(horizontalOrientation_ - amount, 2.0 * M_PI);
}

void FirstPersonTransform::updatePhysicalObject(const ref_ptr<PhysicalObject> &po, GLdouble dt) {
	auto rigidBody = po->rigidBody();
	if (!rigidBody.get()) return;

	bool isMoving = moveForward_ || moveBackward_ || moveLeft_ || moveRight_;

	switch (physicsMode_) {
		case CHARACTER: {
			btVector3 newVelocity(0, 0, 0);
			if (isMoving) {
				newVelocity = btVector3(-step_.x * physicsSpeedFactor_, -step_.y * physicsSpeedFactor_,
										-step_.z * physicsSpeedFactor_);
			}
			rigidBody->activate(true);
			rigidBody->setLinearVelocity(newVelocity);
			break;
		}
		case IMPULSE: {
			if (isMoving) {
				btVector3 impulse(-step_.x * physicsSpeedFactor_, -step_.y * physicsSpeedFactor_,
								  -step_.z * physicsSpeedFactor_);
				rigidBody->activate(true);
				rigidBody->applyCentralImpulse(impulse);
			}
			break;
		}
	}

	btTransform transform;
	btQuaternion rotation;
	rigidBody->getMotionState()->getWorldTransform(transform);
	rotation.setRotation(btVector3(0, 1, 0), horizontalOrientation_);
	transform.setRotation(rotation);
	rigidBody->getMotionState()->setWorldTransform(transform);
}

void FirstPersonTransform::setTransform(const Vec3f &pos, const Vec3f &dir) {
    Vec3f normalizedDir = dir;
    normalizedDir.normalize();
	pos_ = pos;
    horizontalOrientation_ = atan2(-normalizedDir.x, normalizedDir.z);
    verticalOrientation_ = asin(-normalizedDir.y);
}

void FirstPersonTransform::animate(GLdouble dt) {
	rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_);
	Vec3f d = rot_.rotate(Vec3f::front());

	dirXZ_ = Vec3f(d.x, 0.0f, d.z);
	dirXZ_.normalize();
	dirSidestep_ = dirXZ_.cross(Vec3f::up());

	bool hasPhysics = (mesh_.get() && !mesh_->physicalObjects().empty());

	step_ = Vec3f(0.0f);
	if (moveForward_) stepForward(moveAmount_ * dt);
	else if (moveBackward_) stepBackward(moveAmount_ * dt);
	if (moveLeft_) stepLeft(moveAmount_ * dt);
	else if (moveRight_) stepRight(moveAmount_ * dt);

	if (hasPhysics) {
		lock();
		for (const auto &po: mesh_->physicalObjects()) {
			updatePhysicalObject(po, dt);
		}
		unlock();
	} else {
		lock();
		pos_ += step_;
		unlock();
	}

	isMoving_ = moveForward_ || moveBackward_ || moveLeft_ || moveRight_;
}

void FirstPersonTransform::glAnimate(RenderState *rs, GLdouble dt) {
	lock();
	{
		// Simple rotation matrix around up vector (0,1,0)
		GLdouble cy = cos(horizontalOrientation_), sy = sin(horizontalOrientation_);
		matVal_.x[0] = cy;
		matVal_.x[2] = sy;
		matVal_.x[8] = -sy;
		matVal_.x[10] = cy;
		// Translate to mesh position
		const Mat4f &m = mat_->getVertex(0);
		matVal_.x[12] = m.x[12] - pos_.x;
		matVal_.x[13] = m.x[13] - pos_.y;
		matVal_.x[14] = m.x[14] - pos_.z;
		pos_ = Vec3f(0.0f);

		mat_->setVertex(0, matVal_);
	}
	unlock();
}

////////////////
////////////////
////////////////

FirstPersonCameraTransform::FirstPersonCameraTransform(
		const ref_ptr<Camera> &cam,
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<ModelTransformation> &transform,
		const Vec3f &meshEyeOffset,
		GLdouble meshHorizontalOrientation)
		: FirstPersonTransform(transform->get(), mesh),
		  CameraUpdater(cam),
		  mesh_(mesh),
		  mat_(transform->get()),
		  meshEyeOffset_(meshEyeOffset) {
	verticalOrientation_ = 0.0;
	meshHorizontalOrientation_ = meshHorizontalOrientation;
	pos_ = Vec3f(0.0f);
}

FirstPersonCameraTransform::FirstPersonCameraTransform(
		const ref_ptr<Camera> &cam)
		: FirstPersonTransform(cam->view(), ref_ptr<Mesh>()),
		  CameraUpdater(cam) {
	verticalOrientation_ = 0.0;
	meshHorizontalOrientation_ = 0.0;
	pos_ = cam->position()->getVertex(0);
}

void FirstPersonCameraTransform::updateCameraPosition() {
	if (mat_.get()) {
		camPos_ = meshEyeOffset_;
		if (mesh_.get()) {
			camPos_ += mesh_->centerPosition();
		}
		camPos_ = (mat_->getVertex(0) ^ Vec4f(camPos_, 1.0)).xyz_();
	} else {
		camPos_ = pos_;
	}
}

void FirstPersonCameraTransform::updateCameraOrientation() {
	rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_ + meshHorizontalOrientation_);
	camDir_ = rot_.rotate(Vec3f::front());
	rot_.setAxisAngle(dirSidestep_, verticalOrientation_);
	camDir_ = rot_.rotate(camDir_);
}

#define REGEN_ORIENT_THRESHOLD_ 0.1

void FirstPersonCameraTransform::lookUp(GLdouble amount) {
	verticalOrientation_ = math::clamp(verticalOrientation_ + amount,
									   -0.5 * M_PI + REGEN_ORIENT_THRESHOLD_, 0.5 * M_PI - REGEN_ORIENT_THRESHOLD_);
}

void FirstPersonCameraTransform::lookDown(GLdouble amount) {
	verticalOrientation_ = math::clamp(verticalOrientation_ - amount,
									   -0.5 * M_PI + REGEN_ORIENT_THRESHOLD_, 0.5 * M_PI - REGEN_ORIENT_THRESHOLD_);
}

void FirstPersonCameraTransform::zoomIn(GLdouble amount) {}

void FirstPersonCameraTransform::zoomOut(GLdouble amount) {}

void FirstPersonCameraTransform::animate(GLdouble dt) {
	FirstPersonTransform::animate(dt);
	lock();
	{
		updateCameraPosition();
		updateCameraOrientation();
		computeMatrices(camPos_, camDir_);
	}
	unlock();
}

void FirstPersonCameraTransform::glAnimate(RenderState *rs, GLdouble dt) {
	if (mat_.get()) {
		FirstPersonTransform::glAnimate(rs, dt);
	}
	lock();
	{
		updateCamera(camPos_, camDir_, dt);
	}
	unlock();
}

////////////////
////////////////
////////////////

ThirdPersonCameraTransform::ThirdPersonCameraTransform(
		const ref_ptr<Camera> &cam,
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<ModelTransformation> &transform,
		const Vec3f &eyeOffset,
		GLfloat eyeOrientation)
		: FirstPersonCameraTransform(cam, mesh, transform, eyeOffset, eyeOrientation),
		  meshDistance_(10.0f) {
}

void ThirdPersonCameraTransform::updateCameraPosition() {
	FirstPersonCameraTransform::updateCameraPosition();
	meshPos_ = camPos_;

	rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_ + meshHorizontalOrientation_);
	Vec3f dir = rot_.rotate(Vec3f::front());

	rot_.setAxisAngle(dir.cross(Vec3f::up()), verticalOrientation_);
	camPos_ -= rot_.rotate(dir * meshDistance_);
}

void ThirdPersonCameraTransform::updateCameraOrientation() {
	camDir_ = meshPos_ - camPos_;
	camDir_.normalize();
}

void ThirdPersonCameraTransform::zoomIn(GLdouble amount) {
	meshDistance_ = math::clamp(meshDistance_ - amount, 0.0, 100.0);
}

void ThirdPersonCameraTransform::zoomOut(GLdouble amount) {
	meshDistance_ = math::clamp(meshDistance_ + amount, 0.0, 100.0);
}
