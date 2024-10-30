/*
 * qt-camera-events.cpp
 *
 *  Created on: Oct 26, 2014
 *      Author: daniel
 */

#include <GL/glew.h>
#include <QtOpenGL/QGLWidget>
#include <QtCore/QThread>

#include <regen/application.h>
#include "qt-camera-events.h"

using namespace regen;

QtFirstPersonEventHandler::QtFirstPersonEventHandler(
		const ref_ptr<CameraController> &m,
		const std::vector<CameraCommandMapping> &keyMappings)
		: EventHandler(),
		  m_(m),
		  buttonPressed_(GL_FALSE),
		  sensitivity_(0.0002f) {
	for (const auto &x: keyMappings) {
		keyMappings_[x.key] = x;
	}
}

void QtFirstPersonEventHandler::set_sensitivity(GLfloat val) { sensitivity_ = val; }

void QtFirstPersonEventHandler::call(EventObject *evObject, EventData *data) {
	if (data->eventID == Application::MOUSE_MOTION_EVENT) {
		if (buttonPressed_) {
			auto *ev = (Application::MouseMotionEvent *) data;
			Vec2f delta((float) ev->dx, (float) ev->dy);
			m_->lookLeft(delta.x * sensitivity_);
			m_->lookUp(delta.y * sensitivity_);
		}
	}
	else if (data->eventID == Application::KEY_EVENT) {
		auto *ev = (Application::KeyEvent *) data;
		auto evKey = QKeySequence(ev->key).toString();
		auto needle = keyMappings_.find(evKey.toStdString());

		CameraCommand cmd = CameraCommand::NONE;
		if (needle != keyMappings_.end()) {
			cmd = needle->second.command;
		} else {
			if (ev->key == Qt::Key_W || ev->key == Qt::Key_Up) cmd = CameraCommand::MOVE_FORWARD;
			else if (ev->key == Qt::Key_S || ev->key == Qt::Key_Down) cmd = CameraCommand::MOVE_BACKWARD;
			else if (ev->key == Qt::Key_A || ev->key == Qt::Key_Left) cmd = CameraCommand::MOVE_LEFT;
			else if (ev->key == Qt::Key_D || ev->key == Qt::Key_Right) cmd = CameraCommand::MOVE_RIGHT;
			else if (ev->key == Qt::Key_Space) cmd = CameraCommand::JUMP;
		}

		if (cmd != CameraCommand::NONE) {
			if (cmd == CameraCommand::MOVE_FORWARD) m_->moveForward(!ev->isUp);
			else if (cmd == CameraCommand::MOVE_BACKWARD) m_->moveBackward(!ev->isUp);
			else if (cmd == CameraCommand::MOVE_LEFT) m_->moveLeft(!ev->isUp);
			else if (cmd == CameraCommand::MOVE_RIGHT) m_->moveRight(!ev->isUp);
			else if (cmd == CameraCommand::JUMP && !ev->isUp) m_->jump();
		}
	}
	else if (data->eventID == Application::BUTTON_EVENT) {
		auto *ev = (Application::ButtonEvent *) data;
		if (ev->button == Application::MOUSE_BUTTON_LEFT) {
			buttonPressed_ = ev->pressed;
		} else if (ev->button == Application::MOUSE_WHEEL_UP) {
			m_->zoomIn(0.5);
		} else if (ev->button == Application::MOUSE_WHEEL_DOWN) {
			m_->zoomOut(0.5);
		}
	}
}

