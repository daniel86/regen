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
#include <QtCore/QTimer>

const QGL::FormatOptions glFormat =
    QGL::SingleBuffer
   |QGL::NoDepthBuffer
   |QGL::NoAlphaChannel
   |QGL::NoAccumBuffer
   |QGL::NoStencilBuffer
   |QGL::NoStereoBuffers
   |QGL::NoSampleBuffers;

QTGLWidget::QTGLWidget(QtOGLEApplication *app, QWidget *parent)
: QGLWidget(QGLFormat(glFormat),parent),
  app_(app)
{
  setMouseTracking(true);
  setAutoBufferSwap(false);

  redrawTimer_ = new QTimer(); // XXX delete
  QObject::connect(redrawTimer_, SIGNAL(timeout()), this, SLOT(update()));
  redrawTimer_->start(10);
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

static GLint qtToOgleButton(Qt::MouseButton button)
{
  switch(button) {
  case Qt::LeftButton:
    return 1;
  case Qt::RightButton:
    return 2;
  case Qt::MiddleButton:
    return 3;
  case Qt::XButton1:
    return 4;
  case Qt::XButton2:
    return 5;
  case Qt::NoButton:
  case Qt::MouseButtonMask:
  default:
    return -1;
  }
}

void QTGLWidget::mouseClickEvent(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick)
{
  // TODO: invert y ?
  GLint x=event->x(), y=event->y();
  GLint button = qtToOgleButton(event->button());
  if(button==-1) { return; }
  // TODO: mousewheel ?
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

void QTGLWidget::mouseMoveEvent(QMouseEvent *event)
{
  // TODO: invert y ?
  GLint x=event->x(), y=event->y();
  app_->mouseMove(x,y);
}

void QTGLWidget::keyPressEvent(QKeyEvent* event)
{
  int evKey = event->key();
  app_->keyDown(evKey,app_->mouseX(),app_->mouseY());
}
void QTGLWidget::keyReleaseEvent(QKeyEvent *event)
{
  int evKey = event->key();
  app_->keyUp(evKey,app_->mouseX(),app_->mouseY());
}
