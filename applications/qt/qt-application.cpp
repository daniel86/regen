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
  shaderInputWidget_(NULL)
{
  glWidget_.setMinimumSize(100,100);
  glWidget_.setFocusPolicy(Qt::StrongFocus);
  resizeGL(Vec2i(width,height));
}
QtApplication::~QtApplication()
{
  if(shaderInputWidget_!=NULL) {
    delete shaderInputWidget_;
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
  if(shaderInputWidget_==NULL) {
    shaderInputWidget_ = new ShaderInputWidget(NULL);
    QSize oldSize = glWidget_.size();
    QSize widgetSize = shaderInputWidget_->size();

    QWidget *parent = glWidget_.parentWidget();

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setObjectName(QString::fromUtf8("shaderInputLayout"));
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(&glWidget_, 1);
    layout->addWidget(shaderInputWidget_, 0);

    if(parent) {
      QWidget *container = new QWidget(parent);
      container->setLayout(layout);
      container->show();
    }
    else {
      // create a new toplevel window
      QMainWindow *win = new QMainWindow(NULL);
      win->setWindowTitle(glWidget_.windowTitle());
      QWidget *container = new QWidget(NULL);
      container->setLayout(layout);
      win->setCentralWidget(container);
      win->resize(oldSize.width()+widgetSize.width(), oldSize.height());
      win->show();
    }

    shaderInputWidget_->show();
    glWidget_.show();
  }
  in->set_isConstant(GL_FALSE);
  shaderInputWidget_->add(
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
