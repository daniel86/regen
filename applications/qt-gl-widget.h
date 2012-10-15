/*
 * qt-gl-widget.h
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#ifndef QT_GL_WIDGET_H_
#define QT_GL_WIDGET_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <QtOpenGL/QGLWidget>
#include <applications/ogle-application.h>

class GLWidget : public QGLWidget
{
  Q_OBJECT // must include this if you use Qt signals/slots

public:
  GLWidget(
      OGLEApplication *app,
      QGLFormat &format,
      QWidget *parent = NULL);

protected:
  OGLEApplication *app_;

  void initializeGL();
  void resizeGL(int w, int h);
  virtual void paintGL();

  void mouseReleaseEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void keyPressEvent(QKeyEvent *event);
};

#endif /* QT_GL_WIDGET_H_ */
