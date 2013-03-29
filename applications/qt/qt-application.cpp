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
  glContainer_(parent),
  glWidget_(this, &glContainer_),
  shaderInputWidget_(&glContainer_)
{
  glWidget_.setMinimumSize(100,100);
  glWidget_.setFocusPolicy(Qt::StrongFocus);

  QHBoxLayout *layout = new QHBoxLayout();
  layout->setObjectName(QString::fromUtf8("shaderInputLayout"));
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(&glWidget_, 1);
  layout->addWidget(&shaderInputWidget_,0);
  glContainer_.setLayout(layout);
  glContainer_.resize(width+shaderInputWidget_.width(), height);
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
  shaderInputWidget_.hide();
  glContainer_.show();
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
  glWidget_.startRendering();

  int exitCode = app_.exec();

  glWidget_.stopRendering();

  INFO_LOG("Exiting with status " << exitCode << ".");
  return exitCode;
}

void QtApplication::addShaderInput(
    const string &treePath,
    const ref_ptr<ShaderInput> &in,
    const Vec4f &minBound,
    const Vec4f &maxBound,
    const Vec4i &precision,
    const string &description)
{
  in->set_isConstant(GL_FALSE);
  shaderInputWidget_.add(
      treePath,
      in,
      minBound,
      maxBound,
      precision,
      description);
  shaderInputWidget_.show();
}

QWidget* QtApplication::toplevelWidget()
{
  QWidget *p = &glWidget_;
  while(p->parentWidget()!=NULL) { p=p->parentWidget(); }
  return p;
}

QWidget& QtApplication::glWidgetContainer()
{ return glContainer_; }
QTGLWidget& QtApplication::glWidget()
{ return glWidget_; }
