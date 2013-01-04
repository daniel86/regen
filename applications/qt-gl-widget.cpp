/*
 * qt-gl-widget.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include "qt-gl-widget.h"
#include "qt-ogle-application.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QWheelEvent>
#include <QtCore/QTimer>

static GLint qtToOgleButton(Qt::MouseButton button)
{
  switch(button) {
  case Qt::LeftButton:
    return OGLE_MOUSE_BUTTON_LEFT;
  case Qt::RightButton:
    return OGLE_MOUSE_BUTTON_RIGHT;
  case Qt::MiddleButton:
    return OGLE_MOUSE_BUTTON_MIDDLE;
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

#define __REDRAW_DT_ 10

QTGLWidget::QTGLWidget(QtOGLEApplication *app, QWidget *parent)
: QGLWidget(QGLFormat(glFormat),parent),
  app_(app)
{
  setMouseTracking(true);
  setAutoBufferSwap(false);

  redrawTimer_ = new QTimer();
  QObject::connect(redrawTimer_, SIGNAL(timeout()), this, SLOT(update()));
  redrawTimer_->start(__REDRAW_DT_);
}

QTGLWidget::~QTGLWidget()
{
  delete redrawTimer_;
}

void QTGLWidget::initializeGL()
{
  app_->initGL();
}

void QTGLWidget::resizeGL(int w, int h)
{
  app_->resizeGL(w, h);
}

void QTGLWidget::paintEvent(QPaintEvent *event)
{
  makeCurrent();
  app_->drawGL();
}

void QTGLWidget::mouseClickEvent(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick)
{
  GLint x=event->x(), y=event->y();
  GLint button = qtToOgleButton(event->button());
  if(button==-1) { return; }
  app_->mouseButton(button, isPressed, x, y, isDoubleClick);
}
void QTGLWidget::mousePressEvent(QMouseEvent *event)
{
  mouseClickEvent(event, GL_TRUE, GL_FALSE);
}
void QTGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  mouseClickEvent(event, GL_TRUE, GL_TRUE);
}
void QTGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
  mouseClickEvent(event, GL_FALSE, GL_FALSE);
}
void QTGLWidget::mouseWheelEvent(QWheelEvent *event)
{
  GLint x=event->x(), y=event->y();
  GLint button = event->delta()>1 ? OGLE_MOUSE_WHEEL_UP : OGLE_MOUSE_WHEEL_DOWN;
  app_->mouseButton(button, GL_TRUE, x, y, GL_FALSE);
}

void QTGLWidget::mouseMoveEvent(QMouseEvent *event)
{
  GLint x=event->x(), y=event->y();
  app_->mouseMove(x,y);
}

void QTGLWidget::keyPressEvent(QKeyEvent* event)
{
  app_->keyDown(event->key(),app_->mouseX(),app_->mouseY());
}
void QTGLWidget::keyReleaseEvent(QKeyEvent *event)
{
  app_->keyUp(event->key(),app_->mouseX(),app_->mouseY());
}
