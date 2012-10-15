/*
 * qt-gl-widget.cpp
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#include <QtGui/QMouseEvent>

#include "qt-gl-widget.h"

GLWidget::GLWidget(
    OGLEApplication *app,
    QGLFormat &format,
    QWidget *parent)
: QGLWidget(format, parent),
  app_(app)
{
  setMouseTracking(true);
}
void GLWidget::initializeGL()
{
  app_->initGL();
}
void GLWidget::resizeGL(int w, int h)
{
  app_->resizeGL(w,h);
}
void GLWidget::paintGL()
{
  app_->drawGL();
}
static GLint qtToOgleButton(Qt::MouseButton button)
{
  switch(button)
  {
  case Qt::LeftButton:
    return 0;
  case Qt::RightButton:
    return 1;
  case Qt::MiddleButton:
    return 2;
  default:
    return -1;
  }
}
void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
  app_->mouseButton(qtToOgleButton(event->button()),
      GL_FALSE, event->x(), height()-event->y());
}
void GLWidget::mousePressEvent(QMouseEvent *event)
{
  app_->mouseButton(qtToOgleButton(event->button()),
      GL_TRUE, event->x(), height()-event->y());
}
void GLWidget::wheelEvent(QWheelEvent *event)
{
  app_->mouseButton(event->delta()>0 ? 3 : 4,
      GL_FALSE, event->x(), height()-event->y());
}
void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
  app_->mouseMove(event->x(), height()-event->y());
}
void GLWidget::keyPressEvent(QKeyEvent *event)
{
  // TODO: handle keys
  switch(event->key()) {
  case Qt::Key_Escape:
    //exitMainLoop();
      break;
  default:
      event->ignore();
      break;
  }
}

