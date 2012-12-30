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

static int shortcutEater(int event) {
  if(event == FL_SHORTCUT) return 1;
  return 0;
}
static void changeValueCallbackf_(Fl_Widget *widget, void *data) {
  Fl_Valuator *valueWidget = (Fl_Valuator*)widget;
  InputCallbackData *cbData = (InputCallbackData*) data;
  GLfloat *v = (GLfloat*) cbData->value;
  *v = (GLfloat) valueWidget->value();
  cbData->app->valueChanged(cbData->name);
}
static void changeValueCallbacki_(Fl_Widget *widget, void *data) {
  Fl_Valuator *valueWidget = (Fl_Valuator*)widget;
  InputCallbackData *cbData = (InputCallbackData*) data;
  GLint *v = (GLint*) cbData->value;
  *v = (GLint) valueWidget->value();
  cbData->app->valueChanged(cbData->name);
}
static void closeApplicationCallback_(Fl_Widget *widget, void *data)
{
  OGLEFltkApplication *app = (OGLEFltkApplication*) data;
  app->exitMainLoop(0);
}

OGLEFltkApplication::OGLEFltkApplication(
    OGLERenderTree *tree,
    int &argc, char** argv,
    GLuint width, GLuint height)
: OGLEApplication(tree,argc,argv,width,height),
  windowTitle_("OpenGL Engine"),
  fltkHeight_(height),
  fltkWidth_(width),
  mainWindow_(width,height),
  mainWindowPackH_(NULL),
  isApplicationRunning_(GL_TRUE)
{
  lastButtonTime_ = lastMotionTime_;

  Fl::scheme("GTK+");
  // clearlook background
  Fl::background(0xed, 0xec, 0xeb);
  Fl::add_handler(shortcutEater);
}

void OGLEFltkApplication::toggleFullscreen()
{
  if(mainWindow_.fullscreen_active()) {
    mainWindow_.fullscreen_off();
  }
  else {
    mainWindow_.fullscreen();
  }
}

void OGLEFltkApplication::createShaderInputWidget()
{
}

void OGLEFltkApplication::addValueChangedHandler(
    const string &value, void (*function)(void*), void *data)
{
  valueChangedHandler_[value].push_back(ValueChangedHandler(function,data));
}
void OGLEFltkApplication::valueChanged(const string &value)
{
  list<ValueChangedHandler> &handler = valueChangedHandler_[value];
  for(list<ValueChangedHandler>::iterator
      it=handler.begin(); it!=handler.end(); ++it)
  {
    it->function(it->data);
  }
}

void OGLEFltkApplication::addShaderInput(const string &name)
{
  static const int windowWidth = 340;
  static const int windowHeight = 600;
  static const int labelHeight = 24;

  if(uniformWindow_==NULL) {
    uniformWindow_ = new Fl_Window(windowWidth,windowHeight);
    uniformWindow_->callback(closeApplicationCallback_, this);
    uniformWindow_->end();
    uniformWindow_->show();

    uniformScroll_ = new Fl_Scroll(0,0,windowWidth,windowHeight);
    uniformScrollY_ = 0;
    uniformWindow_->add(uniformScroll_);
  }

  Fl_Box *nameWidget = new Fl_Box(0,uniformScrollY_,uniformScroll_->w()-20,labelHeight);
  nameWidget->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
  nameWidget->label(name.c_str());
  uniformScroll_->add(nameWidget);
  uniformScrollY_ += labelHeight;
}

void OGLEFltkApplication::addShaderInputf(
    const string &name,
    list<GLfloat*> values,
    GLfloat min, GLfloat max,
    GLint precision)
{
  static const int valuatorHeight = 24;

  addShaderInput(name);

  for(list<GLfloat*>::iterator it=values.begin(); it!=values.end(); ++it)
  {
    GLfloat *v = *it;
    InputCallbackData *data = new InputCallbackData;
    data->value = v;
    data->app = this;
    data->name = name;
    Fl_Hor_Value_Slider *valueWidget =
        new Fl_Hor_Value_Slider(0,uniformScrollY_,uniformScroll_->w()-20,valuatorHeight);
    valueWidget->bounds(min, max);
    valueWidget->precision(precision);
    valueWidget->value(*v);
    valueWidget->callback(changeValueCallbackf_, data);
    uniformScroll_->add(valueWidget);
    uniformScrollY_ += valuatorHeight;
  }
}

void OGLEFltkApplication::addShaderInputi(
    const string &name,
    list<GLint*> values,
    GLint min, GLint max)
{
  static const int valuatorHeight = 24;

  addShaderInput(name);

  for(list<GLint*>::iterator it=values.begin(); it!=values.end(); ++it)
  {
    GLint *v = *it;
    InputCallbackData *data = new InputCallbackData;
    data->value = v;
    data->app = this;
    data->name = name;
    Fl_Hor_Value_Slider *valueWidget =
        new Fl_Hor_Value_Slider(0,uniformScrollY_,uniformScroll_->w()-20,valuatorHeight);
    valueWidget->bounds(min, max);
    valueWidget->precision(0);
    valueWidget->value(*v);
    valueWidget->callback(changeValueCallbacki_, data);
    uniformScroll_->add(valueWidget);
    uniformScrollY_ += valuatorHeight;
  }
}

void OGLEFltkApplication::addShaderInput(
    const ref_ptr<ShaderInput1f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex1f(0)));
  addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(
    const ref_ptr<ShaderInput2f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex2f(0).x));
  values.push_back(&(in->getVertex2f(0).y));
  addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(
    const ref_ptr<ShaderInput3f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex3f(0).x));
  values.push_back(&(in->getVertex3f(0).y));
  values.push_back(&(in->getVertex3f(0).z));
  addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(
    const ref_ptr<ShaderInput4f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  list<GLfloat*> values;
  values.push_back(&(in->getVertex4f(0).x));
  values.push_back(&(in->getVertex4f(0).y));
  values.push_back(&(in->getVertex4f(0).z));
  values.push_back(&(in->getVertex4f(0).w));
  addShaderInputf(in->name(), values, min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(const ref_ptr<ShaderInput1i> &in, GLint min, GLint max)
{
  list<GLint*> values;
  values.push_back(&(in->getVertex1i(0)));
  addShaderInputi(in->name(), values, min, max);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(const ref_ptr<ShaderInput2i> &in, GLint min, GLint max)
{
  list<GLint*> values;
  values.push_back(&(in->getVertex2i(0).x));
  values.push_back(&(in->getVertex2i(0).y));
  addShaderInputi(in->name(), values, min, max);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(const ref_ptr<ShaderInput3i> &in, GLint min, GLint max)
{
  list<GLint*> values;
  values.push_back(&(in->getVertex3i(0).x));
  values.push_back(&(in->getVertex3i(0).y));
  values.push_back(&(in->getVertex3i(0).z));
  addShaderInputi(in->name(), values, min, max);
  in->set_isConstant(GL_FALSE);
}
void OGLEFltkApplication::addShaderInput(const ref_ptr<ShaderInput4i> &in, GLint min, GLint max)
{
  list<GLint*> values;
  values.push_back(&(in->getVertex4i(0).x));
  values.push_back(&(in->getVertex4i(0).y));
  values.push_back(&(in->getVertex4i(0).z));
  values.push_back(&(in->getVertex4i(0).w));
  addShaderInputi(in->name(), values, min, max);
  in->set_isConstant(GL_FALSE);
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
  OGLEFltkApplication *app = (OGLEFltkApplication*) data;
  app->postRedisplay();
}

void OGLEFltkApplication::setKeepAspect()
{
  mainWindow_.size_range(
      fltkWidth_/10, fltkHeight_/10,
      fltkWidth_*10, fltkHeight_*10,
      0, 0,
      1);
}

void OGLEFltkApplication::setFixedSize()
{
  mainWindow_.size_range(
      fltkWidth_, fltkHeight_,
      fltkWidth_, fltkHeight_,
      0, 0, 0);
}

void OGLEFltkApplication::postRedisplay()
{
  fltkWindow_->redraw();
}

void OGLEFltkApplication::initGL()
{
  mainWindow_.begin();
  mainWindow_.callback(closeApplicationCallback_, this);

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

  Fl::add_idle(_postRedisplay, this);
}

void OGLEFltkApplication::resize(GLuint width, GLuint height)
{
  mainWindow_.size((GLint)width, (GLint)height);
  fltkHeight_ = height;
  fltkWidth_ =  width;
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
  switch(ev) {
  case FL_KEYDOWN: {
    app_->keyDown(Fl::event_key(), Fl::event_x(),  Fl::event_y());
    return 1;
  }
  case FL_SHORTCUT:
    switch(Fl::event_key()) {
    case FL_Escape:
      app_->exitMainLoop(0);
      return 1;
    default:
      return Fl_Gl_Window::handle(ev);
    }
  case FL_KEYUP: {
    unsigned char key = Fl::event_key();
    if(key == 'f') {
      app_->toggleFullscreen();
    }
    else {
      app_->keyUp(Fl::event_key(), Fl::event_x(), Fl::event_y());
    }
    return 1;
  }
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
  case FL_NO_EVENT:
  case FL_HIDE:
  case FL_ENTER:
  case FL_LEAVE:
    return Fl_Gl_Window::handle(ev);
  default:
    return Fl_Gl_Window::handle(ev);
  }
}
