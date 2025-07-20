#include "regen/camera/camera-controller-base.h"

using namespace regen;

CameraControllerBase::CameraControllerBase(const ref_ptr<Camera> &cam)
		: cam_(cam) {
}

void CameraControllerBase::computeMatrices(const Vec3f &pos, const Vec3f &dir) {
	view_ = Mat4f::lookAtMatrix(pos, dir, Vec3f::up());
	viewInv_ = view_.lookAtInverse();
	viewproj_ = view_ * cam_->projection()->getVertex(0).r;
	viewprojInv_ = cam_->projectionInverse()->getVertex(0).r * viewInv_;
	cam_->frustum()[0].update(pos, dir);
}

void CameraControllerBase::updateCamera(const Vec3f &pos, const Vec3f &dir, GLdouble dt) {
	velocity_ = (lastPosition_ - pos) / dt;
	lastPosition_ = pos;

	cam_->view()->setVertex(0, view_);
	cam_->viewInverse()->setVertex(0, viewInv_);
	cam_->viewProjection()->setVertex(0, viewproj_);
	cam_->viewProjectionInverse()->setVertex(0, viewprojInv_);
	cam_->position()->setVertex3(0, pos);
	cam_->direction()->setVertex3(0, dir);
	cam_->velocity()->setVertex3(0, velocity_);
	cam_->nextStamp();

	if (cam_->isAudioListener()) {
		AudioListener::set3f(AL_POSITION, pos);
		AudioListener::set3f(AL_VELOCITY, velocity_);
		AudioListener::set6f(AL_ORIENTATION, Vec6f(dir, Vec3f::up()));
	}
}
