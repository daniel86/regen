/*
 * ogle-application.cpp
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <ogle/external/glsw/glsw.h>

#include "ogle-application.h"
#include "ogle-render-tree.h"

GLuint OGLEApplication::BUTTON_EVENT =
    EventObject::registerEvent("ogleButton");
GLuint OGLEApplication::KEY_EVENT =
    EventObject::registerEvent("ogleKey");
GLuint OGLEApplication::MOUSE_MOTION_EVENT =
    EventObject::registerEvent("ogleMouseMtion");
GLuint OGLEApplication::RESIZE_EVENT =
    EventObject::registerEvent("ogleResize");

OGLEApplication::OGLEApplication(
    OGLERenderTree *renderTree,
    int &argc, char** argv,
    GLuint width, GLuint height)
: EventObject(),
  renderTree_(renderTree),
  glSize_(width,height),
  waitForVSync_(GL_FALSE),
  lastMouseX_(0u),
  lastMouseY_(0u)
{
  lastMotionTime_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastDisplayTime_ = lastMotionTime_;

  Logging::addLogger( new FileLogger(Logging::INFO, "ogle-info.log") );
  Logging::addLogger( new FileLogger(Logging::DEBUG, "ogle-debug.log") );
  Logging::addLogger( new FileLogger(Logging::WARN, "ogle-error.log") );
  Logging::addLogger( new FileLogger(Logging::ERROR, "ogle-error.log") );
  Logging::addLogger( new FileLogger(Logging::FATAL, "ogle-error.log") );
  Logging::addLogger( new CoutLogger(Logging::INFO ) );
  Logging::addLogger( new CoutLogger(Logging::DEBUG) );
  Logging::addLogger( new CerrLogger(Logging::FATAL) );
  Logging::addLogger( new CerrLogger(Logging::ERROR) );
  Logging::addLogger( new CerrLogger(Logging::WARN) );
  Logging::set_verbosity(Logging::V);

  renderTree->setWindowSize(width,height);
}

void OGLEApplication::setWaitForVSync(GLboolean v)
{
  waitForVSync_ = v;
}

const Vec2ui& OGLEApplication::windowSize() const
{
  return glSize_;
}
GLuint OGLEApplication::windowWidth() const
{
  return glSize_.x;
}
GLuint OGLEApplication::windowHeight() const
{
  return glSize_.y;
}
GLuint OGLEApplication::mouseX() const
{
  return lastMouseX_;
}
GLuint OGLEApplication::mouseY() const
{
  return lastMouseY_;
}

void OGLEApplication::mouseMove(GLint x, GLint y)
{
  boost::posix_time::ptime time(
      boost::posix_time::microsec_clock::local_time());
  GLdouble milliSeconds = ((GLdouble)(time - lastMotionTime_).total_microseconds())/1000.0;
  GLint dx = x - lastMouseX_;
  GLint dy = y - lastMouseY_;
  lastMouseX_ = x;
  lastMouseY_ = y;
  renderTree_->setMousePosition(x,y);

  MouseMotionEvent event;
  event.dt = milliSeconds;
  event.dx = dx;
  event.dy = dy;
  emitEvent(MOUSE_MOTION_EVENT, &event);

  lastMotionTime_ = time;
}

void OGLEApplication::mouseButton(GLuint button, GLboolean pressed, GLuint x, GLuint y)
{
  lastMouseX_ = x;
  lastMouseY_ = y;
  renderTree_->setMousePosition(x,y);

  ButtonEvent event;
  event.button = button;
  event.x = x;
  event.y = y;
  event.pressed = pressed;
  emitEvent(BUTTON_EVENT, &event);
}

void OGLEApplication::keyUp(unsigned char key, GLuint x, GLuint y) {
  KeyEvent event;
  event.isUp = GL_TRUE;
  event.key = key;
  event.x = x;
  event.y = y;
  emitEvent(KEY_EVENT, &event);
}
void OGLEApplication::keyDown(unsigned char key, GLuint x, GLuint y) {
  KeyEvent event;
  event.isUp = GL_FALSE;
  event.key = key;
  event.x = x;
  event.y = y;
  emitEvent(KEY_EVENT, &event);
}

void OGLEApplication::initTree()
{
  renderTree_->initTree();
}

void OGLEApplication::initGL()
{
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    cerr << "Error: " << glewGetErrorString(err) << endl;
    exit(1);
  }
  glswInit();

  // FIXME: hardcoded path
#ifdef WIN32
  glswSetPath("ogle\\shader", ".glsl");
#else
  glswSetPath("/home/daniel/coding/cpp/ogle-3d/ogle/shader/", ".glsl");
#endif

  DEBUG_LOG("VENDOR: " << glGetString(GL_VENDOR));
  DEBUG_LOG("RENDERER: " << glGetString(GL_RENDERER));
  DEBUG_LOG("VERSION: " << glGetString(GL_VERSION));

#define DEBUG_GLi(dname, pname) { \
    GLint val; \
    glGetIntegerv(pname, &val); \
    DEBUG_LOG(dname << ": " << val); }
  DEBUG_GLi("MAX_DRAW_BUFFERS", GL_MAX_DRAW_BUFFERS);
  DEBUG_GLi("MAX_LIGHTS", GL_MAX_LIGHTS);
  DEBUG_GLi("MAX_TEXTURE_IMAGE_UNITS", GL_MAX_TEXTURE_IMAGE_UNITS);
  DEBUG_GLi("MAX_TEXTURE_SIZE", GL_MAX_TEXTURE_SIZE);
  DEBUG_GLi("MAX_TEXTURE_UNITS", GL_MAX_TEXTURE_UNITS);
  DEBUG_GLi("MAX_VERTEX_ATTRIBS", GL_MAX_VERTEX_ATTRIBS);
#undef DEBUG_GLi

  if (!glewIsSupported("GL_EXT_framebuffer_object"))
  {
    ERROR_LOG("GL_EXT_framebuffer_object unsupported.");
    exit(-1);
  }

  // set some default states
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glDisable(GL_NORMALIZE);
  // specify whether front- or back-facing facets can be culled
  glCullFace(GL_BACK);
  // define front- and back-facing polygons
  glFrontFace(GL_CCW);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  glViewport(0, 0, glSize_.x, glSize_.y);
}

void OGLEApplication::resizeGL(GLuint width, GLuint height)
{
  glSize_.x = width;
  glSize_.y = height;
  glViewport(0, 0, glSize_.x, glSize_.y);
  renderTree_->setWindowSize(glSize_.x, glSize_.y);
  emitEvent(RESIZE_EVENT);
}

void OGLEApplication::drawGL()
{
  static const GLdouble VSYNC_RATE = 1000.0/60.0;
  boost::posix_time::ptime t(
      boost::posix_time::microsec_clock::local_time());
  GLdouble dt = ((GLdouble)(t-lastDisplayTime_).total_microseconds())/1000.0;
  if(waitForVSync_ && dt < VSYNC_RATE) { return; }
  lastDisplayTime_ = t;
  renderTree_->render(dt);

  swapGL();

  renderTree_->postRender(dt);
}
