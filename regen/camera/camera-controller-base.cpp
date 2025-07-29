#include "regen/camera/camera-controller-base.h"

using namespace regen;

CameraControllerBase::CameraControllerBase(const ref_ptr<Camera> &cam)
		: cam_(cam) {
}

void CameraControllerBase::computeMatrices(const Vec3f &pos, const Vec3f &dir) {
	cam_->setView(0, Mat4f::lookAtMatrix(pos, dir, Vec3f::up()));
	auto &view = cam_->view(0);
	cam_->setViewInverse(0, view.lookAtInverse());
	auto &viewInv = cam_->viewInverse(0);

	cam_->setViewProjection(0, view * cam_->projection(0));
	cam_->setViewProjectionInverse(0, cam_->projectionInverse(0) * viewInv);
	cam_->frustum()[0].update(pos, dir);
	cam_->updateFrustumBuffer();
}

void CameraControllerBase::updateCamera(const Vec3f &pos, const Vec3f &dir, float dt) {
	cam_->setPosition(0, pos);
	cam_->setDirection(0, dir);
	cam_->updateShaderData(dt);
}
