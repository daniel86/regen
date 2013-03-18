/*
 * qt-ogle-application.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <ogle/animations/animation-manager.h>

#include "qt-application.h"
using namespace ogle;

static char *appArgs[] = {};
static int appArgCount = 0;

QtApplication::QtApplication(
    const ref_ptr<RootNode> &tree,
    int &argc, char** argv,
    GLuint width, GLuint height,
    QWidget *parent)
: OGLEApplication(tree,argc,argv,width,height),
  app_(appArgCount,(char**)appArgs),
  glWidget_(this, parent),
  genericDataWindow_(NULL)
{}
QtApplication::~QtApplication()
{
  if(genericDataWindow_!=NULL) {
    delete genericDataWindow_;
  }
}

void QtApplication::set_windowTitle(const string &title)
{
  QWidget *p = &glWidget_;
  while(p->parentWidget()!=NULL) { p=p->parentWidget(); }
  p->setWindowTitle(QString(title.c_str()));
}

void QtApplication::toggleFullscreen()
{
  QWidget *p = &glWidget_;
  while(p->parentWidget()!=NULL) { p=p->parentWidget(); }

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
void QtApplication::swapGL()
{
  glWidget_.swapBuffers();
}

int QtApplication::mainLoop()
{
  AnimationManager::get().resume();
  return app_.exec();
}

void QtApplication::addGenericData(
    const string &treePath,
    const ref_ptr<ShaderInput> &in,
    const Vec4f &minBound,
    const Vec4f &maxBound,
    const Vec4i &precision,
    const string &description)
{
  if(genericDataWindow_==NULL) {
    genericDataWindow_ = new GenericDataWindow(&glWidget_);
    genericDataWindow_->show();
    QObject::connect(genericDataWindow_, SIGNAL(windowClosed()), &app_, SLOT(quit()));
  }
  genericDataWindow_->addGenericData(
      treePath,
      in,
      minBound,
      maxBound,
      precision,
      description);
}

QTGLWidget& QtApplication::glWidget()
{ return glWidget_; }
