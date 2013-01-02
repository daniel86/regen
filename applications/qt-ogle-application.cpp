/*
 * qt-ogle-application.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <ogle/animations/animation-manager.h>
#include "qt-ogle-application.h"

QtOGLEApplication::QtOGLEApplication(
    OGLERenderTree *tree,
    int &argc, char** argv,
    GLuint width, GLuint height,
    QWidget *parent)
: OGLEApplication(tree,argc,argv,width,height),
  app_(argc,argv),
  glWidget_(this, parent)
{
}

void QtOGLEApplication::show()
{
  glWidget_.show();
  initTree();
}

int QtOGLEApplication::mainLoop()
{
  AnimationManager::get().resume();
  return app_.exec();
}

void QtOGLEApplication::exitMainLoop(int errorCode)
{
  app_.exit(errorCode);
}

void QtOGLEApplication::swapGL()
{
  glWidget_.swapBuffers();
}

void QtOGLEApplication::set_windowTitle(const string &title)
{
  QWidget *p = &glWidget_;
  while(p->parentWidget()!=NULL) { p=p->parentWidget(); }
  p->setWindowTitle(QString(title.c_str()));
}

void QtOGLEApplication::resize(GLuint width, GLuint height)
{
  glWidget_.resize(width, height);
}
