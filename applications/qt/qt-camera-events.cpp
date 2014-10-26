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
    const ref_ptr<FirstPersonCameraTransform> &m)
: EventHandler(),
  m_(m),
  buttonPressed_(GL_FALSE),
  sensitivity_(0.0002f)
{
}

void QtFirstPersonEventHandler::set_sensitivity(GLfloat val)
{ sensitivity_ = val; }

void QtFirstPersonEventHandler::call(EventObject *evObject, EventData *data)
{
  if(data->eventID == Application::MOUSE_MOTION_EVENT) {
    if(buttonPressed_) {
      Application::MouseMotionEvent *ev = (Application::MouseMotionEvent*)data;
      Vec2f delta((float)ev->dx, (float)ev->dy);
      m_->lookLeft(delta.x*sensitivity_);
      m_->lookUp(delta.y*sensitivity_);
    }
  }
  else if(data->eventID == Application::KEY_EVENT) {
    Application::KeyEvent *ev = (Application::KeyEvent*)data;
    if(ev->key == Qt::Key_W || ev->key == Qt::Key_Up)         m_->moveForward(!ev->isUp);
    else if(ev->key == Qt::Key_S || ev->key == Qt::Key_Down)  m_->moveBackward(!ev->isUp);
    else if(ev->key == Qt::Key_A || ev->key == Qt::Key_Left)  m_->moveLeft(!ev->isUp);
    else if(ev->key == Qt::Key_D || ev->key == Qt::Key_Right) m_->moveRight(!ev->isUp);
  }
  else if(data->eventID == Application::BUTTON_EVENT) {
    Application::ButtonEvent *ev = (Application::ButtonEvent*)data;
    if(ev->button == Application::MOUSE_BUTTON_LEFT) {
      buttonPressed_ = ev->pressed;
    }
    else if(ev->button == Application::MOUSE_WHEEL_UP) {
      m_->zoomIn(0.5);
    }
    else if(ev->button == Application::MOUSE_WHEEL_DOWN) {
      m_->zoomOut(0.5);
    }
  }
}

