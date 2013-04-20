/*
 * qt-application.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <QDesktopWidget>

#include <regen/config.h>
#include <regen/animations/animation-manager.h>

#include "qt-application.h"
using namespace regen;

#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
#define EVENT_PROCESSING_INTERVAL 20000
#endif

// strange QT argc/argv handling
static const char *appArgs[] = {"dummy"};
static int appArgCount = 1;

QtApplication::QtApplication(
    int &argc, char** argv,
    const QGLFormat &glFormat,
    GLuint width, GLuint height,
    QWidget *parent)
: Application(argc,argv), isMainloopRunning_(GL_FALSE), exitCode_(0)
{
  app_ = new QApplication(appArgCount,(char**)appArgs);

  glContainer_ = new QWidget(parent);
  glWidget_ = new QTGLWidget(this, glFormat, glContainer_);
  glWidget_->setMinimumSize(100,100);
  glWidget_->setFocusPolicy(Qt::StrongFocus);

  shaderInputWidget_ = new ShaderInputWidget(glContainer_);

  QHBoxLayout *layout = new QHBoxLayout();
  layout->setObjectName(QString::fromUtf8("shaderInputLayout"));
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(glWidget_, 1);
  layout->addWidget(shaderInputWidget_,0);
  glContainer_->setLayout(layout);
  glContainer_->resize(width+shaderInputWidget_->width(), height);
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
  shaderInputWidget_->hide();
  glContainer_->show();
  glWidget_->show();
  glWidget_->setFocus();
}

void QtApplication::exitMainLoop(int errorCode)
{
  exitCode_ = errorCode;
  isMainloopRunning_ = GL_FALSE;
}
int QtApplication::mainLoop()
{
  AnimationManager::get().resume();
  isMainloopRunning_ = GL_TRUE;

  toplevelWidget()->installEventFilter(glWidget_);

#ifdef SINGLE_THREAD_GUI_AND_GRAPHICS
  glWidget_->run();
#else
  glWidget_->startRendering();
  while(isMainloopRunning_)
  {
    app_->processEvents();
#ifdef UNIX
    usleep(EVENT_PROCESSING_INTERVAL);
#else
    boost::this_thread::sleep(boost::posix_time::microseconds(EVENT_PROCESSING_INTERVAL));
#endif
  }
  glWidget_->stopRendering();
#endif

  INFO_LOG("Exiting with status " << exitCode_ << ".");
  return exitCode_;
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
  shaderInputWidget_->add(
      treePath,
      in,
      minBound,
      maxBound,
      precision,
      description);
  shaderInputWidget_->show();
}

QWidget* QtApplication::toplevelWidget()
{
  QWidget *p = glWidget_;
  while(p->parentWidget()!=NULL) { p=p->parentWidget(); }
  return p;
}

QWidget* QtApplication::glWidgetContainer()
{ return glContainer_; }
QTGLWidget* QtApplication::glWidget()
{ return glWidget_; }
