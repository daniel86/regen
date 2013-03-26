/*
 * qt-gl-widget.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QWheelEvent>

#include "qt-gl-widget.h"
#include "qt-application.h"
using namespace regen;

static GLint qtToOgleButton(Qt::MouseButton button)
{
  switch(button) {
  case Qt::LeftButton:
    return Application::MOUSE_BUTTON_LEFT;
  case Qt::RightButton:
    return Application::MOUSE_BUTTON_RIGHT;
  case Qt::MiddleButton:
    return Application::MOUSE_BUTTON_MIDDLE;
  case Qt::XButton1:
  case Qt::XButton2:
  case Qt::NoButton:
  case Qt::MouseButtonMask:
  default:
    return -1;
  }
}

const QGL::FormatOptions glFormat =
    QGL::SingleBuffer
   |QGL::NoDepthBuffer
   |QGL::NoAlphaChannel
   |QGL::NoAccumBuffer
   |QGL::NoStencilBuffer
   |QGL::NoStereoBuffers
   |QGL::NoSampleBuffers;

QTGLWidget::QTGLWidget(QtApplication *app, QWidget *parent)
: QGLWidget(QGLFormat(glFormat),parent),
  app_(app)
{
  setMouseTracking(true);
  setAutoBufferSwap(false);

  QObject::connect(&redrawTimer_, SIGNAL(timeout()), this, SLOT(updateGL()));
  // seems qt needs some time for event processing
  redrawTimer_.start(10);
}

void QTGLWidget::setUpdateInterval(GLint interval)
{
  redrawTimer_.setInterval(interval);
}

void QTGLWidget::initializeGL()
{  app_->initGL(); }
void QTGLWidget::resizeGL(int w, int h)
{ app_->resizeGL(Vec2i(w,h)); }

void QTGLWidget::paintGL()
{
  app_->drawGL();
  swapBuffers();
  app_->updateGL();
}

void QTGLWidget::mouseClick__(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick)
{
  GLint x=event->x(), y=event->y();
  GLint button = qtToOgleButton(event->button());
  if(button==-1) { return; }
  Application::ButtonEvent ev;
  ev.button = button;
  ev.isDoubleClick = isDoubleClick;
  ev.pressed = isPressed;
  ev.x = x;
  ev.y = y;
  app_->mouseButton(ev);
  event->accept();
}
void QTGLWidget::mousePressEvent(QMouseEvent *event)
{
  mouseClick__(event, GL_TRUE, GL_FALSE);
  event->accept();
}
void QTGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  mouseClick__(event, GL_TRUE, GL_TRUE);
  event->accept();
}
void QTGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
  mouseClick__(event, GL_FALSE, GL_FALSE);
  event->accept();
}
void QTGLWidget::wheelEvent(QWheelEvent *event)
{
  GLint x=event->x(), y=event->y();
  GLint button = event->delta()>0 ? Application::MOUSE_WHEEL_UP : Application::MOUSE_WHEEL_DOWN;
  Application::ButtonEvent ev;
  ev.button = button;
  ev.isDoubleClick = GL_FALSE;
  ev.pressed = GL_FALSE;
  ev.x = x;
  ev.y = y;
  app_->mouseButton(ev);
  event->accept();
}

void QTGLWidget::mouseMoveEvent(QMouseEvent *event)
{
  app_->mouseMove(Vec2i(event->x(),event->y()));
  event->accept();
}

void QTGLWidget::keyPressEvent(QKeyEvent* event)
{
  const Vec2f &mousePos = app_->mousePosition()->getVertex2f(0);
  Application::KeyEvent ev;
  ev.key = event->key();
  ev.x = (GLint)mousePos.x;
  ev.y = (GLint)mousePos.y;
  app_->keyDown(ev);
  event->accept();
}
void QTGLWidget::keyReleaseEvent(QKeyEvent *event)
{
  switch(event->key()) {
  case Qt::Key_Escape:
    app_->exitMainLoop(0);
    break;
  case Qt::Key_F:
    app_->toggleFullscreen();
    break;
  default: {
    const Vec2f &mousePos = app_->mousePosition()->getVertex2f(0);
    Application::KeyEvent ev;
    ev.key = event->key();
    ev.x = (GLint)mousePos.x;
    ev.y = (GLint)mousePos.y;
    app_->keyUp(ev);
    break;
  }
  }
  event->accept();
}
