/*
 * qt-gl-widget.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef QT_GL_WIDGET_H_
#define QT_GL_WIDGET_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <QtOpenGL/QGLWidget>
#include <QtCore/QTimer>

namespace ogle {

class QtApplication; // forward declaration

/**
 * QT Widget that can be used for OpenGL rendering.
 */
class QTGLWidget : public QGLWidget
{
  Q_OBJECT

public:
  QTGLWidget(QtApplication *app, QWidget *parent=NULL);

  /**
   * @param interval update interval in milliseconds.
   */
  void setUpdateInterval(GLint interval);

protected:
  QtApplication *app_;
  QTimer redrawTimer_;

  void initializeGL();
  void paintGL();
  void resizeGL(int width, int height);

  void mousePressEvent(QMouseEvent *event);
  void mouseDoubleClickEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);

  void mouseMoveEvent(QMouseEvent *event);

  void keyPressEvent(QKeyEvent *event);
  void keyReleaseEvent(QKeyEvent *event);

  void mouseClick__(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick);
};

}

#endif /* QT_GL_WIDGET_H_ */
