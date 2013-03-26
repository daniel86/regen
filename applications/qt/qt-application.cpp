/*
 * qt-application.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <QDesktopWidget>

#include <regen/animations/animation-manager.h>

#include "qt-application.h"
using namespace regen;

// strange QT argc/argv handling
static const char *appArgs[] = {"dummy"};
static int appArgCount = 1;

QtApplication::QtApplication(
    int &argc, char** argv,
    GLuint width, GLuint height,
    QWidget *parent)
: Application(argc,argv),
  app_(appArgCount,(char**)appArgs),
  glWidget_(this, parent),
  shaderInputWindow_(NULL)
{
  resizeGL(Vec2i(width,height));
}
QtApplication::~QtApplication()
{
  if(shaderInputWindow_!=NULL) {
    delete shaderInputWindow_;
  }
}

void QtApplication::toggleFullscreen()
{
  QWidget *p = toplevelWidget();
  if(p->isFullScreen())
  { p->showNormal(); }
  else
  { p->showFullScreen(); }
}

void QtApplication::show()
{
  glWidget_.show();
  glWidget_.setFocus();
}

void QtApplication::exitMainLoop(int errorCode)
{
  app_.exit(errorCode);
}
int QtApplication::mainLoop()
{
  AnimationManager::get().resume();
  return app_.exec();
}

void QtApplication::addShaderInput(
    const string &treePath,
    const ref_ptr<ShaderInput> &in,
    const Vec4f &minBound,
    const Vec4f &maxBound,
    const Vec4i &precision,
    const string &description)
{
  if(shaderInputWindow_==NULL) {
    shaderInputWindow_ = new ShaderInputWindow(&glWidget_);
    shaderInputWindow_->show();
    QObject::connect(shaderInputWindow_, SIGNAL(windowClosed()), &app_, SLOT(quit()));
  }
  in->set_isConstant(GL_FALSE);
  shaderInputWindow_->add(
      treePath,
      in,
      minBound,
      maxBound,
      precision,
      description);
}

QWidget* QtApplication::toplevelWidget()
{
  QWidget *p = &glWidget_;
  while(p->parentWidget()!=NULL) { p=p->parentWidget(); }
  return p;
}

QTGLWidget& QtApplication::glWidget()
{ return glWidget_; }
