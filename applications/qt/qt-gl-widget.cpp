/*
 * qt-gl-widget.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QCloseEvent>
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

QTGLWidget::QTGLWidget(
    QtApplication *app,
    const QGLFormat &glFormat,
    QWidget *parent)
: QGLWidget(glFormat,parent),
  app_(app),
  renderThread_(this),
  updateInterval_(16), // ~60FPS
  isRunning_(GL_FALSE)
{
  setMouseTracking(true);
  setAutoBufferSwap(false);
}

void QTGLWidget::setUpdateInterval(GLint interval)
{
  updateInterval_ = interval;
}

void QTGLWidget::resizeEvent(QResizeEvent *ev)
{
  if(!isRunning_) {
    // QGLWidget wants to do the first resize...
    QGLWidget::resizeEvent(ev);
  }
  else {
    resizeGL(ev->size().width(), ev->size().height());
  }
}
void QTGLWidget::paintEvent(QPaintEvent *ev)
{
}

// init GL in main thread
void QTGLWidget::initializeGL()
{ app_->initGL(); }
// queue resize event to be processed in render thread
void QTGLWidget::resizeGL(int w, int h)
{ app_->resizeGL(Vec2i(w,h)); }
void QTGLWidget::paintGL()
{}
void QTGLWidget::updateGL()
{}

void QTGLWidget::startRendering()
{
  doneCurrent();
  renderThread_.start(QThread::HighPriority);
}
void QTGLWidget::stopRendering()
{
  isRunning_ = GL_FALSE;
  renderThread_.wait();
}

QTGLWidget::GLThread::GLThread(QTGLWidget *glWidget)
: QThread(), glWidget_(glWidget)
{
}
void QTGLWidget::GLThread::run()
{
  if(glWidget_->isRunning_) {
    WARN_LOG("Render thread already running.");
    return;
  }
  glWidget_->isRunning_ = GL_TRUE;
  GLint dt;

  glWidget_->makeCurrent();
  while(glWidget_->isRunning_)
  {
    glWidget_->app_->drawGL();
    // not sure why swap buffers is needed. we are using single
    // buffer gl context....
    glWidget_->swapBuffers();
    glWidget_->app_->updateGL();
    // flush GL draw calls
    glFlush();
    // adjust interval to hit the desired frame rate if we can
    boost::posix_time::ptime t(
        boost::posix_time::microsec_clock::local_time());
    dt = ((GLuint)(t- glWidget_->app_->lastDisplayTime_).total_microseconds())/1000.0;
    // sleep desired interval
    msleep(max(0,glWidget_->updateInterval_-dt));
  }
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

bool QTGLWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Close) {
    app_->exitMainLoop(0);
    return true;
  }
  return QObject::eventFilter(obj, event);
}
