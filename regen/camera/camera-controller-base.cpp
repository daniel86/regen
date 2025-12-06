#include "regen/camera/camera-controller-base.h"

using namespace regen;

CameraControllerBase::CameraControllerBase(const ref_ptr<Camera> &cam)
		: cam_(cam) {
}

void CameraControllerBase::computeMatrices(const Vec3f &pos, const Vec3f &dir) {
	const auto view = Mat4f::lookAtMatrix(pos, dir, Vec3f::up());
	cam_->setView(0, view);
	cam_->setViewProjection(0, view * cam_->projection(0));

	const auto viewInv = view.lookAtInverse();
	cam_->setViewInverse(0, viewInv);
	cam_->setViewProjectionInverse(0, cam_->projectionInverse(0) * viewInv);

	cam_->frustum()[0].update(pos, dir);
	cam_->updateFrustumBuffer();
}

void CameraControllerBase::updateCamera(const Vec3f &pos, const Vec3f &dir, float dt) {
	cam_->setPosition(0, pos);
	cam_->setDirection(0, dir);
	cam_->updateShaderData(dt);
}
