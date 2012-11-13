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
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>

#include "fltk-ogle-application.h"

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
  isApplicationRunning_(GL_TRUE),
  windowTitle_("OpenGL Engine")
{
  lastButtonTime_ = lastMotionTime_;

  Fl::scheme("GTK+");
  // clearlook background
  Fl::background(0xed, 0xec, 0xeb);

  createShaderInputWidget();
}

void OGLEFltkApplication::createShaderInputWidget()
{
}

static void changeValueCallbackf_(Fl_Widget *widget, void *data) {
  Fl_Valuator *valueWidget = (Fl_Valuator*)widget;
  GLfloat *v = (GLfloat*) data;
  *v = (GLfloat) valueWidget->value();
}

static void _addShaderInputf(
    const string &name,
    list<GLfloat*> values,
    GLfloat min, GLfloat max,
    GLint precision)
{
  static const int windowHeight = 600;
  static const int windowWidth = 340;
  static const int labelHeight = 24;
  static const int valuatorHeight = 24;

  static Fl_Window *window = NULL;
  static Fl_Scroll *scroll = NULL;
  static int y = 0;

  if(window==NULL) {
    window = new Fl_Window(windowWidth,windowHeight);
    window->end();
    window->show();

    scroll = new Fl_Scroll(0,0,windowWidth,windowHeight);
    window->add(scroll);
  }

  Fl_Box *nameWidget = new Fl_Box(0,y,windowWidth-20,labelHeight);
  nameWidget->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
  nameWidget->label(name.c_str());
  scroll->add(nameWidget);
  y += labelHeight;

  for(list<GLfloat*>::iterator it=values.begin(); it!=values.end(); ++it) {
    GLfloat *v = *it;

    Fl_Hor_Value_Slider *valueWidget = new Fl_Hor_Value_Slider(0,y,windowWidth-20,valuatorHeight);
    valueWidget->bounds(min, max);
    valueWidget->precision(precision);
    valueWidget->value(*v);
    valueWidget->callback(changeValueCallbackf_, v);
    scroll->add(valueWidget);
    y += valuatorHeight;
  }

  /*
  Fl_Box *hline0 = new Fl_Box(0,y,windowWidth,1);
  hline0->box(FL_BORDER_BOX);
  window->add(hline0);
  y += 1;
  */
}

void OGLEFltkApplication::addShaderInput(
    ref_ptr<ShaderInput1f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex1f(0)));
  _addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(
    ref_ptr<ShaderInput2f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex2f(0).x));
  values.push_back(&(in->getVertex2f(0).y));
  _addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(
    ref_ptr<ShaderInput3f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex3f(0).x));
  values.push_back(&(in->getVertex3f(0).y));
  values.push_back(&(in->getVertex3f(0).z));
  _addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(
    ref_ptr<ShaderInput4f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex4f(0).x));
  values.push_back(&(in->getVertex4f(0).y));
  values.push_back(&(in->getVertex4f(0).z));
  values.push_back(&(in->getVertex4f(0).w));
  _addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(ref_ptr<ShaderInput1i> &in,
    GLint min, GLint max, GLint step)
{
cout << "ADD " << in->name() << endl;
}
void OGLEFltkApplication::addShaderInput(ref_ptr<ShaderInput2i> &in,
    const Vec2i& min, const Vec2i& max, const Vec2i& step)
{
cout << "ADD " << in->name() << endl;
}
void OGLEFltkApplication::addShaderInput(ref_ptr<ShaderInput3i> &in,
    const Vec3i& min, const Vec3i& max, const Vec3i& step)
{
cout << "ADD " << in->name() << endl;
}
void OGLEFltkApplication::addShaderInput(ref_ptr<ShaderInput4i> &in,
    const Vec4i& min, const Vec4i& max, const Vec4i& step)
{
cout << "ADD " << in->name() << endl;
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
  isApplicationRunning_ = GL_FALSE;
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
  AnimationManager::get().resume();

  while(Fl::wait() && isApplicationRunning_) {}
  return 0;
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
  if(ev==FL_NO_EVENT) {
    return Fl_Gl_Window::handle(ev);
  }
  switch(ev) {
  case FL_KEYDOWN:
    app_->keyDown(
        (unsigned char) Fl::event_key(),
        Fl::event_x(),
        Fl::event_y());
    return 1;
  case FL_KEYUP:
    app_->keyUp(
        (unsigned char) Fl::event_key(),
        Fl::event_x(),
        Fl::event_y());
    return 1;
  case FL_PUSH:
  case FL_RELEASE:
    app_->mouseButton(
        fltkButtonToOgleButton(Fl::event_button()),
        (ev==FL_PUSH ? GL_TRUE : GL_FALSE),
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
  case FL_ENTER:
  case FL_LEAVE:
    return Fl_Gl_Window::handle(ev);
  default:
    return Fl_Gl_Window::handle(ev);
  }
}
