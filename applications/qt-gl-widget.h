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

class QtOGLEApplication; // forward declaration

/**
 * QT Widget that can be used for OpenGL rendering.
 */
class QTGLWidget : public QGLWidget
{
  Q_OBJECT

public:
  QTGLWidget(QtOGLEApplication *app, QWidget *parent=NULL);
  ~QTGLWidget();

protected:
  QtOGLEApplication *app_;
  QTimer *redrawTimer_;

  void initializeGL();

  void paintEvent(QPaintEvent *event);

  void resizeGL(int width, int height);

  void mouseClickEvent(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick);
  void mousePressEvent(QMouseEvent *event);
  void mouseDoubleClickEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseWheelEvent(QWheelEvent *event);

  void mouseMoveEvent(QMouseEvent *event);

  void keyPressEvent(QKeyEvent *event);
  void keyReleaseEvent(QKeyEvent *event);
};

#endif /* QT_GL_WIDGET_H_ */
