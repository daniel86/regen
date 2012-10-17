/*
 * fltk-application.cpp
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <ogle/font/font-manager.h>
#include <ogle/utility/logging.h>
#include <ogle/utility/gl-error.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/external/glsw/glsw.h>

#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Pack.H>

#include "fltk-ogle-application.h"

////////////////////

/*
void OGLEGlutApplication::mousePassiveMotionStatic(int x, int y)
{
  mouseMotionStatic(x,y);
}
void OGLEGlutApplication::mouseMotionStatic(int x, int y)
{
  singleton_->mouseMove(x,singleton_->glutHeight_-y);
}
void OGLEGlutApplication::mouseButtonStatic(int button, int state, int x, int y)
{
  singleton_->mouseButton(button,state==GLUT_DOWN,x,singleton_->glutHeight_-y);
}
*/

///////////////////

//////

OGLEFltkApplication::OGLEFltkApplication(
    OGLERenderTree *tree,
    int &argc, char** argv,
    GLuint width, GLuint height)
: OGLEApplication(tree,argc,argv,width,height),
  fltkHeight_(height),
  fltkWidth_(width),
  mainWindow_(width,height),
  mainWindowPackH_(NULL),
  windowTitle_("OpenGL Engine")
{
  lastButtonTime_ = lastMotionTime_;

  Fl::scheme("GTK+");
  // clearlook background
  Fl::background(0xed, 0xec, 0xeb);
}

void OGLEFltkApplication::createWidgets(Fl_Pack *parent)
{
  fltkWindow_ = new GLWindow(this,0,0,256,256);
  parent->resizable(fltkWindow_);
}

void OGLEFltkApplication::set_windowTitle(const string &windowTitle)
{
  windowTitle_ = windowTitle;
  mainWindow_.label(windowTitle_.c_str());
}

static void _postRedisplay(void *data)
{
  Fl_Gl_Window *win = (Fl_Gl_Window*) data;
  win->redraw();
}

void OGLEFltkApplication::initGL()
{
  mainWindow_.begin();

  mainWindowPackH_ = new Fl_Pack(0,0,fltkWidth_,fltkHeight_);
  mainWindowPackH_->type(Fl_Pack::HORIZONTAL);
  mainWindowPackH_->begin();

  mainWindowPackV_ = new Fl_Pack(0,0,fltkWidth_,fltkHeight_);
  mainWindowPackV_->type(Fl_Pack::VERTICAL);
  mainWindowPackV_->begin();

  createWidgets(mainWindowPackV_);
  if(fltkWindow_==NULL) {
    fltkWindow_ = new GLWindow(this,0,0,fltkWidth_,fltkHeight_);
    mainWindowPackV_->resizable(fltkWindow_);
  }

  mainWindowPackV_->end();

  mainWindowPackH_->end();
  mainWindowPackH_->resizable(mainWindowPackV_);

  mainWindow_.end();
  mainWindow_.resizable(mainWindowPackH_);

  mainWindow_.show();
  fltkWindow_->make_current();
  OGLEApplication::initGL();

  Fl::add_idle(_postRedisplay, fltkWindow_);
}

void OGLEFltkApplication::exitMainLoop(int errorCode)
{
  // TODO: do better...
  exit(0);
}

void OGLEFltkApplication::show()
{
  initGL();
  initTree();
}

void OGLEFltkApplication::swapGL()
{
  fltkWindow_->swap_buffers();
}

int OGLEFltkApplication::mainLoop()
{
  // TODO: debug tree
  //debugTree(renderTree_->globalStates().get(), "  ");
  AnimationManager::get().resume();

  return Fl::run();
}

////////////

OGLEFltkApplication::GLWindow::GLWindow(
    OGLEFltkApplication *app,
    GLint x, GLint y,
    GLint w, GLint h)
: Fl_Gl_Window(x,y,w,h),
  app_(app)
{
  mode(FL_SINGLE);
  //mode(FL_RGB8);
  //mode(FL_RGB);
}

void OGLEFltkApplication::GLWindow::flush()
{
  draw();
}

void OGLEFltkApplication::GLWindow::draw()
{
  app_->drawGL();
}

void OGLEFltkApplication::GLWindow::resize(int x, int y, int w, int h)
{
  cout << "LAYOUT " << w << " , " << h << endl;
  Fl_Gl_Window::resize(x,y,w,h);
  app_->resizeGL(w, h);
}

static int fltkButtonToOgleButton(int button)
{
  switch(button) {
  case FL_MIDDLE_MOUSE:
    return 2;
  case FL_RIGHT_MOUSE:
    return 1;
  case FL_LEFT_MOUSE:
  default:
    return 0;
  }
}
int OGLEFltkApplication::GLWindow::handle(int ev)
{
  switch(ev) {
  case FL_PUSH:
  case FL_RELEASE:
    app_->mouseButton(
        fltkButtonToOgleButton(Fl::event_button()),
        Fl::event_is_click()>0 ? GL_TRUE : GL_FALSE,
        Fl::event_x(),
        Fl::event_y());
    return 1;
  case FL_MOUSEWHEEL:
    app_->mouseButton(
        Fl::event_dy()<0 ? 3 : 4,
        GL_FALSE,
        Fl::event_x(),
        Fl::event_y());
    return 1;
  case FL_MOVE:
  case FL_DRAG:
    app_->mouseMove(
        Fl::event_x(),
        Fl::event_y());
    return 1;
  default:
    return Fl_Gl_Window::handle(ev);
  }
}
