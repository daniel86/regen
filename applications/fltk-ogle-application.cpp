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

#include <fltk/run.h>

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

OGLEFltkApplication::OGLEFltkApplication(
    OGLERenderTree *tree,
    int &argc, char** argv,
    GLuint width, GLuint height)
: OGLEApplication(tree,argc,argv,width,height),
  fltkHeight_(width),
  fltkWidth_(height),
  fltkWindow_(this,0u,0u,width,height,"OpenGL Engine")
{
  lastButtonTime_ = lastMotionTime_;
}

void OGLEFltkApplication::set_windowTitle(const string &windowTitle)
{
  fltkWindow_.label(windowTitle.c_str());
}

void OGLEFltkApplication::initGL()
{
  OGLEApplication::initGL();
}

void OGLEFltkApplication::exitMainLoop(int errorCode)
{
}

void OGLEFltkApplication::show()
{
  fltkWindow_.show();
  initGL();
  initTree();
}

void OGLEFltkApplication::swapGL()
{
  //glutSwapBuffers();
  //glutPostRedisplay();
  //redraw();
}

int OGLEFltkApplication::mainLoop()
{
  // TODO: debug tree
  //debugTree(renderTree_->globalStates().get(), "  ");
  AnimationManager::get().resume();


  return fltk::run();
}

////////////

OGLEFltkApplication::GLWindow::GLWindow(
    OGLEFltkApplication *app,
    GLuint x, GLuint y,
    GLuint w, GLuint h,
    const string &title)
: fltk::GlWindow(x,y,w,h,title.c_str()),
  app_(app)
{
}

void OGLEFltkApplication::GLWindow::flush()
{
  draw();
}

void OGLEFltkApplication::GLWindow::draw()
{
  app_->drawGL();
}

void OGLEFltkApplication::GLWindow::layout()
{
  app_->resizeGL(w(), h());
}

int OGLEFltkApplication::GLWindow::handle(int ev)
{
  // handle() may make OpenGL calls if it first calls GlWindow::make_current().
  // If this is not done, the current OpenGL context may be another window.
  // You should only call non-drawing functions in handle():

  /*
  switch(ev) {
  case fltk::PUSH:
    // mouse down event ...
    // position in fltk::event_x() and fltk::event_y()
    singleton_->mouseButton(1,
        state==GLUT_DOWN,
        fltk::event_x(), fltk::event_y());
    return 1;
  case fltk::DRAG:
    // mouse moved while down event ...
    return 1;
  case fltk::RELEASE:
    // mouse up event ...
    return 1;
  case fltk::FOCUS :
  case fltk::UNFOCUS :
    // Return 1 if you want keyboard events, 0 otherwise
    return 1;
  case fltk::KEY:
    // keypress, key is in fltk::event_key(), ascii in fltk::event_text()
    // Return 1 if you understand/use the keyboard event, 0 otherwise...
    return 1;
  case fltk::SHORTCUT:
    // shortcut, key is in fltk::event_key(), ascii in fltk::event_text()
    // Return 1 if you understand/use the shortcut event, 0 otherwise...
    return 1;
  default:
    */
    // let the base class handle all other events:
    return fltk::GlWindow::handle(ev);
  //}
}
