#include "regen/camera/camera-controller-base.h"

using namespace regen;

CameraControllerBase::CameraControllerBase(const ref_ptr<Camera> &cam)
		: cam_(cam) {
}

void CameraControllerBase::computeMatrices(const Vec3f &pos, const Vec3f &dir) {
	Mat4f &proj = *(Mat4f *) cam_->projection()->ownedClientData();
	Mat4f &projInv = *(Mat4f *) cam_->projectionInverse()->ownedClientData();
	view_ = Mat4f::lookAtMatrix(pos, dir, Vec3f::up());
	viewInv_ = view_.lookAtInverse();
	viewproj_ = view_ * proj;
	viewprojInv_ = projInv * viewInv_;
	cam_->frustum()[0].update(pos, dir);
}

void CameraControllerBase::updateCamera(const Vec3f &pos, const Vec3f &dir, GLdouble dt) {
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
