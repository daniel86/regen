/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include <regen/av/audio.h>

#include "camera-controller.h"

using namespace regen;

CameraController::CameraController(const ref_ptr<Camera> &cam)
		: Animation(false, true),
		  CameraControllerBase(cam),
		  cameraMode_(FIRST_PERSON),
		  meshDistance_(10.0f),
		  lastOrientation_(0.0),
		  hasUpdated_(GL_FALSE) {
	setAnimationName("controller");
	horizontalOrientation_ = 0.0;
	verticalOrientation_ = 0.0;
	meshHorizontalOrientation_ = 0.0;
	moveAmount_ = 1.0;
	moveForward_ = GL_FALSE;
	moveBackward_ = GL_FALSE;
	moveLeft_ = GL_FALSE;
	moveRight_ = GL_FALSE;
	moveUp_ = GL_FALSE;
	moveDown_ = GL_FALSE;
	isMoving_ = GL_FALSE;
	matVal_ = Mat4f::identity();
	#define REGEN_ORIENT_THRESHOLD_ 0.1
	orientThreshold_ = 0.5 * M_PI + REGEN_ORIENT_THRESHOLD_;
	pos_ = cam->position()->getVertex(0).r;
}

void CameraController::setAttachedTo(
		const ref_ptr<ShaderInputMat4> &target,
		const ref_ptr<Mesh> &mesh) {
	attachedToTransform_ = target;
	attachedToMesh_ = mesh;
	pos_ = target->getVertex(0).r.position();
}

void CameraController::stepUp(const GLfloat &v) {
	step(Vec3f(0.0f, v, 0.0f));
}

void CameraController::stepDown(const GLfloat &v) {
	step(Vec3f(0.0f, -v, 0.0f));
}

void CameraController::stepForward(const GLfloat &v) {
	step(dirXZ_ * v);
}

void CameraController::stepBackward(const GLfloat &v) {
	step(dirXZ_ * (-v));
}

void CameraController::stepLeft(const GLfloat &v) {
	step(dirSidestep_ * (-v));
}

void CameraController::stepRight(const GLfloat &v) {
	step(dirSidestep_ * v);
}

void CameraController::step(const Vec3f &v) {
	step_ += v;
}

void CameraController::lookLeft(GLdouble amount) {
	horizontalOrientation_ = fmod(horizontalOrientation_ + amount, 2.0 * M_PI);
}

void CameraController::lookRight(GLdouble amount) {
	horizontalOrientation_ = fmod(horizontalOrientation_ - amount, 2.0 * M_PI);
}

#define REGEN_ORIENT_THRESHOLD_ 0.1

void CameraController::lookUp(GLdouble amount) {
	verticalOrientation_ = math::clamp(verticalOrientation_ + amount, -orientThreshold_, orientThreshold_);
}

void CameraController::lookDown(GLdouble amount) {
	verticalOrientation_ = math::clamp(verticalOrientation_ - amount, -orientThreshold_, orientThreshold_);
}

void CameraController::zoomIn(GLdouble amount) {
	if(isThirdPerson()) {
		meshDistance_ = math::clamp(meshDistance_ - amount, 0.0, 100.0);
	}
}

void CameraController::zoomOut(GLdouble amount) {
	if(isThirdPerson()) {
		meshDistance_ = math::clamp(meshDistance_ + amount, 0.0, 100.0);
	}
}

void CameraController::setTransform(const Vec3f &pos, const Vec3f &dir) {
    Vec3f normalizedDir = dir;
    normalizedDir.normalize();
	pos_ = pos;
    horizontalOrientation_ = atan2(-normalizedDir.x, normalizedDir.z);
    verticalOrientation_ = asin(-normalizedDir.y);
}

void CameraController::updateCameraPosition() {
	if (attachedToTransform_.get()) {
		camPos_ = meshEyeOffset_;
		if (attachedToMesh_.get()) {
			camPos_ += attachedToMesh_->centerPosition();
		}
		camPos_ = (matVal_ ^ Vec4f(camPos_, 1.0)).xyz_();
	} else {
		camPos_ = pos_;
	}
	if (isThirdPerson()) {
		meshPos_ = camPos_;
		rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_ + meshHorizontalOrientation_);
		Vec3f dir = rot_.rotate(Vec3f::front());
		rot_.setAxisAngle(dir.cross(Vec3f::up()), verticalOrientation_);
		camPos_ -= rot_.rotate(dir * meshDistance_);
	}
}

void CameraController::updateCameraOrientation() {
	if (isThirdPerson()) {
		camDir_ = meshPos_ - camPos_;
		camDir_.normalize();
	} else {
		rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_ + meshHorizontalOrientation_);
		camDir_ = rot_.rotate(Vec3f::front());
		rot_.setAxisAngle(dirSidestep_, verticalOrientation_);
		camDir_ = rot_.rotate(camDir_);
	}
}

void CameraController::updateModel() {
	if (attachedToTransform_.get()) {
		// Simple rotation matrix around up vector (0,1,0)
		GLfloat cy = cos(horizontalOrientation_), sy = sin(horizontalOrientation_);
		matVal_.x[0] = cy;
		matVal_.x[2] = sy;
		matVal_.x[8] = -sy;
		matVal_.x[10] = cy;
		// Translate to camera position
		matVal_.x[12] -= pos_.x;
		matVal_.x[13] -= pos_.y;
		matVal_.x[14] -= pos_.z;
		pos_ = Vec3f(0.0f);
	}
}

void CameraController::applyStep(GLfloat dt, const Vec3f &offset) {
	pos_ += offset;
}

void CameraController::jump() {
	// do nothing
}

void CameraController::animate(GLdouble dt) {
	step_ = Vec3f(0.0f);
	isMoving_ = moveForward_ || moveBackward_ || moveLeft_ || moveRight_;
	auto orientation = horizontalOrientation_ + meshHorizontalOrientation_;
	auto hasOrientationChanged = orientation != lastOrientation_;

	if (hasOrientationChanged) {
		rot_.setAxisAngle(Vec3f::up(), orientation);
		Vec3f d = rot_.rotate(Vec3f::front());

		dirXZ_ = Vec3f(d.x, 0.0f, d.z);
		dirXZ_.normalize();
		dirSidestep_ = dirXZ_.cross(Vec3f::up());
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

	{
		// TODO: allow setting hasUpdated_ to false to avoid unnecessary culling computations
		//         but the kinematic controller may change camera position in applyStep below though
		//         e.g. character stands on a moving platform.
		//if (hasOrientationChanged || isMoving_)
		applyStep(dt, step_);
		updateModel();
		updateCameraPosition();
		updateCameraOrientation();
		computeMatrices(camPos_, camDir_);
		if (attachedToTransform_.get()) {
			attachedToTransform_->setVertex(0, matVal_);
		}
		updateCamera(camPos_, camDir_, dt);
	}
	lastOrientation_ = orientation;
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
