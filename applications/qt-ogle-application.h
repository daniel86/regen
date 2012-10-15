/*
 * ogle-qt-application.h
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#ifndef OGLE_QT_APP_H_
#define OGLE_QT_APP_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <QtGui/QApplication>

#include <applications/ogle-application.h>
#include <applications/qt-gl-widget.h>

class OGLEQtApplication : public OGLEApplication
{
public:
  static QGLFormat getDefaultFormat();

  OGLEQtApplication(
      OGLERenderTree *tree,
      int &argc, char** argv,
      QGLFormat format=getDefaultFormat(),
      GLuint width=800, GLuint height=600);

  GLWidget& glWidget() { return glWidget_; }

  virtual void show();
  virtual int mainLoop();
  virtual void exitMainLoop(int errorCode);
  virtual void set_windowTitle(const string&);

protected:
  virtual void swapGL();

  QApplication qtApp_;

  GLWidget glWidget_;
};

#endif // OGLE_QT_APP_H_
