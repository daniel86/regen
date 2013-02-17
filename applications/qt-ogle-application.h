/*
 * qt-ogle-application.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef QT_OGLE_APPLICATION_H_
#define QT_OGLE_APPLICATION_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QApplication>
#include "ogle-application.h"
#include "qt-gl-widget.h"

#include <string>
using namespace std;

/**
 * QT-OGLE application. Uses QApplication's mainloop
 * and QGLWidget for rendering.
 */
class QtOGLEApplication : public OGLEApplication
{
public:
  QtOGLEApplication(
      const ref_ptr<RenderTree> &tree,
      int &argc, char** argv,
      GLuint width=800, GLuint height=600,
      QWidget *parent=NULL);
  virtual ~QtOGLEApplication() {}

  QTGLWidget& glWidget() { return glWidget_; }

  // OGLEApplication override
  virtual void show();
  virtual int mainLoop();
  virtual void exitMainLoop(int errorCode);
  virtual void set_windowTitle(const string&);
  virtual void swapGL();

protected:
  QApplication app_;
  QTGLWidget glWidget_;
};

#endif /* QT_OGLE_APPLICATION_H_ */
