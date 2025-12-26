#include <regen/av/audio.h>

#include "camera-controller.h"

using namespace regen;

namespace regen {
	static constexpr float CAMERA_ORIENT_THRESHOLD = 0.1f;
}

CameraController::CameraController(const ref_ptr<Camera> &cam)
		: Animation(false, true),
		  CameraControllerBase(cam) {
	setAnimationName("controller");
	orientThreshold_ = 0.5 * M_PI + CAMERA_ORIENT_THRESHOLD;
	pos_ = cam->position(0);
}

void CameraController::setAttachedTo(
		const ref_ptr<ModelTransformation> &target,
		const ref_ptr<Mesh> &mesh) {
	attachedToTransform_ = target;
	attachedToMesh_ = mesh;
	pos_ = target->position(0).r;
}

void CameraController::stepUp(const float &v) {
	step(Vec3f(0.0f, v, 0.0f));
}

void CameraController::stepDown(const float &v) {
	step(Vec3f(0.0f, -v, 0.0f));
}

void CameraController::stepForward(const float &v) {
	step(dirXZ_ * v);
}

void CameraController::stepBackward(const float &v) {
	step(dirXZ_ * (-v));
}

void CameraController::stepLeft(const float &v) {
	step(dirSidestep_ * (-v));
}

void CameraController::stepRight(const float &v) {
	step(dirSidestep_ * v);
}

void CameraController::step(const Vec3f &v) {
	step_ += v;
}

void CameraController::lookLeft(double amount) {
	horizontalOrientation_ = fmod(horizontalOrientation_ + amount, math::twoPi<double>());
}

void CameraController::lookRight(double amount) {
	horizontalOrientation_ = fmod(horizontalOrientation_ - amount, math::twoPi<double>());
}

void CameraController::lookUp(double amount) {
	verticalOrientation_ = math::clamp<float>(verticalOrientation_ + amount, -orientThreshold_, orientThreshold_);
}

void CameraController::lookDown(double amount) {
	verticalOrientation_ = math::clamp<float>(verticalOrientation_ - amount, -orientThreshold_, orientThreshold_);
}

void CameraController::zoomIn(double amount) {
	if(isThirdPerson()) {
		meshDistance_ = math::clamp<float>(meshDistance_ - amount, 0.0, 100.0);
	}
}

void CameraController::zoomOut(double amount) {
	if(isThirdPerson()) {
		meshDistance_ = math::clamp<float>(meshDistance_ + amount, 0.0, 100.0);
	}
}

void CameraController::setTransform(const Vec3f &pos, const Vec3f &dir) {
    Vec3f normalizedDir = dir;
    normalizedDir.normalize();
	pos_ = pos;
    horizontalOrientation_ = atan2(-normalizedDir.x, normalizedDir.z);
    verticalOrientation_ = asin(-normalizedDir.y);
}

void CameraController::jump() {
	// do nothing
}

void CameraController::initCameraController() {
	updateCameraPose();
	computeMatrices(camPos_, camDir_);
	updateCamera(camPos_, camDir_, 0.0f);
}

void CameraController::cpuUpdate(double dt) {
	updateStep(dt);
	if (isRotating_ || isMoving_) {
		pos_ += step_;
		updateCameraPose();
		computeMatrices(camPos_, camDir_);
	}
	updateCamera(camPos_, camDir_, dt);
}

void CameraController::updateStep(double dt) {
	step_ = Vec3f::zero();
	isMoving_ = moveForward_ || moveBackward_ || moveLeft_ || moveRight_ || moveUp_ || moveDown_;
	const auto orientation = horizontalOrientation_ + meshHorizontalOrientation_;
	isRotating_ = orientation != lastOrientation_;
	lastOrientation_ = orientation;

	if (isRotating_) {
		rot_.setAxisAngle(Vec3f::up(), orientation);
		Vec3f d = rot_.rotate(Vec3f::front());

		dirXZ_ = Vec3f(d.x, 0.0f, d.z);
		dirXZ_.normalize();
		dirSidestep_ = dirXZ_.cross(Vec3f::up());
		dirSidestep_.normalize();
	}

	if (moveForward_) {
		stepForward(moveAmount_ * dt);
	}
	else if (moveBackward_) {
		stepBackward(moveAmount_ * dt);
	}
	if (moveLeft_) {
		stepLeft(moveAmount_ * dt);
	}
	else if (moveRight_) {
		stepRight(moveAmount_ * dt);
	}
	else if (moveUp_) {
		stepUp(moveAmount_ * dt * 0.5);
	}
	else if (moveDown_) {
		stepDown(moveAmount_ * dt * 0.5);
	}
}

void CameraController::updateCameraPose() {
	if (attachedToTransform_.get()) {
		// Simple rotation matrix around up vector (0,1,0)
		float cy = cos(horizontalOrientation_), sy = sin(horizontalOrientation_);
		matVal_.x[0] = cy;
		matVal_.x[2] = sy;
		matVal_.x[8] = -sy;
		matVal_.x[10] = cy;
		// Translate to camera position
		matVal_.x[12] -= pos_.x;
		matVal_.x[13] -= pos_.y;
		matVal_.x[14] -= pos_.z;
		pos_ = Vec3f::zero();

		if(attachedToTransform_->hasModelMat()) {
			attachedToTransform_->setModelMat(0, matVal_);
		} else {
			attachedToTransform_->setModelOffset(0, matVal_.position());
		}

		camPos_ = meshEyeOffset_;
		if (attachedToMesh_.get()) {
			camPos_ += attachedToMesh_->centerPosition();
		}
		camPos_ = matVal_.mul_t31(camPos_);
	} else {
		camPos_ = pos_;
	}

	if (isThirdPerson()) {
		meshPos_ = camPos_;
		rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_ + meshHorizontalOrientation_);
		Vec3f dir = rot_.rotate(Vec3f::front());
		rot_.setAxisAngle(dir.cross(Vec3f::up()), verticalOrientation_);
		camPos_ -= rot_.rotate(dir * meshDistance_);
		camDir_ = meshPos_ - camPos_;
		camDir_.normalize();
	} else {
		rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_ + meshHorizontalOrientation_);
		camDir_ = rot_.rotate(Vec3f::front());
		rot_.setAxisAngle(dirSidestep_, verticalOrientation_);
		camDir_ = rot_.rotate(camDir_);
	}
}

namespace regen {
	std::ostream &operator<<(std::ostream &out, const CameraCommand &command) {
		switch (command) {
			case CameraCommand::NONE:
				out << "NONE";
				break;
			case CameraCommand::MOVE_FORWARD:
				out << "MOVE_FORWARD";
				break;
			case CameraCommand::MOVE_BACKWARD:
				out << "MOVE_BACKWARD";
				break;
			case CameraCommand::MOVE_LEFT:
				out << "MOVE_LEFT";
				break;
			case CameraCommand::MOVE_RIGHT:
				out << "MOVE_RIGHT";
				break;
			case CameraCommand::MOVE_UP:
				out << "MOVE_UP";
				break;
			case CameraCommand::MOVE_DOWN:
				out << "MOVE_DOWN";
				break;
			case CameraCommand::JUMP:
				out << "JUMP";
				break;
			case CameraCommand::CROUCH:
				out << "CROUCH";
				break;
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, CameraCommand &command) {
		std::string val;
		in >> val;
		boost::to_upper(val);

		if (val == "MOVE_FORWARD") {
			command = CameraCommand::MOVE_FORWARD;
		} else if (val == "MOVE_BACKWARD") {
			command = CameraCommand::MOVE_BACKWARD;
		} else if (val == "MOVE_LEFT") {
			command = CameraCommand::MOVE_LEFT;
		} else if (val == "MOVE_RIGHT") {
			command = CameraCommand::MOVE_RIGHT;
		} else if (val == "MOVE_UP") {
			command = CameraCommand::MOVE_UP;
		} else if (val == "MOVE_DOWN") {
			command = CameraCommand::MOVE_DOWN;
		} else if (val == "JUMP") {
			command = CameraCommand::JUMP;
		} else if (val == "CROUCH") {
			command = CameraCommand::CROUCH;
		} else {
			command = CameraCommand::NONE;
		}

		return in;
	}
}
