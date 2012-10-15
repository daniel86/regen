/*
 * ogle-qt-application.cpp
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#include <ogle/animations/animation-manager.h>

#include "qt-ogle-application.h"

QGLFormat OGLEQtApplication::getDefaultFormat()
{
  QGLFormat format;
  format.setProfile(QGLFormat::CoreProfile);
  format.setVersion(3, 0);
  format.setOption(
       QGL::SingleBuffer
      |QGL::NoDepthBuffer
      |QGL::NoStereoBuffers
      |QGL::NoStencilBuffer
      |QGL::NoSampleBuffers
      |QGL::NoAccumBuffer
      |QGL::NoAlphaChannel
      |QGL::NoDeprecatedFunctions
  );
  return format;
}

OGLEQtApplication::OGLEQtApplication(
    OGLERenderTree *tree,
    int &argc, char** argv,
    QGLFormat format,
    GLuint width, GLuint height)
: OGLEApplication(tree,argc,argv,width,height),
  qtApp_(argc, argv),
  glWidget_(this,format)
{
  glWidget_.resize(width,height);
}

void OGLEQtApplication::set_windowTitle(const string &title)
{
  qtApp_.setApplicationName( QString(title.c_str()) );
}

void OGLEQtApplication::swapGL()
{
  glWidget_.swapBuffers();
  glWidget_.update();
}

void OGLEQtApplication::show()
{
  glWidget_.show();
  OGLEApplication::show();
}

int OGLEQtApplication::mainLoop()
{
  // TODO: debug tree
  //debugTree(renderTree_->globalStates().get(), "  ");
  AnimationManager::get().resume();

  return qtApp_.exec();
}
void OGLEQtApplication::exitMainLoop(int errorCode)
{
  qtApp_.exit(errorCode);
}
