/*
 * ogle-application.cpp
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <ogle/external/glsw/glsw.h>
#include <ogle/config.h>

#include "ogle-application.h"

#define VSYNC_RATE (1000.0/60.0)

GLuint OGLEApplication::BUTTON_EVENT =
    EventObject::registerEvent("ogleButton");
GLuint OGLEApplication::KEY_EVENT =
    EventObject::registerEvent("ogleKey");
GLuint OGLEApplication::MOUSE_MOTION_EVENT =
    EventObject::registerEvent("ogleMouseMtion");
GLuint OGLEApplication::RESIZE_EVENT =
    EventObject::registerEvent("ogleResize");

OGLEApplication::OGLEApplication(
    const ref_ptr<RootNode> &tree,
    int &argc, char** argv,
    GLuint width, GLuint height)
: EventObject(),
  renderTree_(tree),
  glSize_(width,height),
  waitForVSync_(GL_FALSE),
  lastMouseX_(0u),
  lastMouseY_(0u),
  isGLInitialized_(GL_FALSE)
{
  lastMotionTime_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastDisplayTime_ = lastMotionTime_;

  srand(time(0));

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
}

const ref_ptr<RootNode>& OGLEApplication::renderTree() const
{
  return renderTree_;
}

GLboolean OGLEApplication::isGLInitialized() const
{
  return isGLInitialized_;
}

void OGLEApplication::setWaitForVSync(GLboolean v)
{
  waitForVSync_ = v;
}

Vec2ui* OGLEApplication::glSizePtr()
{
  return &glSize_;
}
const Vec2ui& OGLEApplication::glSize() const
{
  return glSize_;
}
GLuint OGLEApplication::glWidth() const
{
  return glSize_.x;
}
GLuint OGLEApplication::glHeight() const
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

  renderTree_->set_mousePosition(
      Vec2f((GLfloat)x, (GLfloat)x));

  MouseMotionEvent event;
  event.dt = milliSeconds;
  event.dx = dx;
  event.dy = dy;
  emitEvent(MOUSE_MOTION_EVENT, &event);

  lastMotionTime_ = time;
}

void OGLEApplication::mouseButton(GLuint button, GLboolean pressed, GLuint x, GLuint y, GLboolean isDoubleClick)
{
  lastMouseX_ = x;
  lastMouseY_ = y;

  ButtonEvent event;
  event.button = button;
  event.x = x;
  event.y = y;
  event.pressed = pressed;
  event.isDoubleClick = isDoubleClick;
  emitEvent(BUTTON_EVENT, &event);
}

void OGLEApplication::keyUp(int key, GLuint x, GLuint y) {
  KeyEvent event;
  event.isUp = GL_TRUE;
  event.key = (unsigned char)key;
  event.keyValue = key;
  event.x = x;
  event.y = y;
  emitEvent(KEY_EVENT, &event);
}
void OGLEApplication::keyDown(int key, GLuint x, GLuint y) {
  KeyEvent event;
  event.isUp = GL_FALSE;
  event.key = (unsigned char)key;
  event.keyValue = key;
  event.x = x;
  event.y = y;
  emitEvent(KEY_EVENT, &event);
}

GLboolean OGLEApplication::setupGLSWPath(const boost::filesystem::path &path)
{
  if(!boost::filesystem::exists(path)) return GL_FALSE;
  GLboolean hasShaderFiles = GL_FALSE;
  GLboolean hasChildShaderFiles = GL_FALSE;

  boost::filesystem::directory_iterator it(path), eod;
  BOOST_FOREACH(const boost::filesystem::path &child, make_pair(it, eod))
  {
    if(is_directory(child)) {
      // check if sub directories contain glsl files
      hasChildShaderFiles |= setupGLSWPath(child);
    }
    else if(!hasShaderFiles && is_regular_file(child)) {
      // check if directory contains glsl files
      boost::filesystem::path ext = child.extension();
      hasShaderFiles = (ext.compare(".glsl")==0);
    }
  }

  if(hasShaderFiles) {
    string includePath(path.c_str());

#ifdef UNIX
    // GLSW seems to want a terminal '/' on unix
    if(*includePath.rbegin()!='/') {
      includePath += "/";
    }
#endif
#ifdef WIN32
    if(*includePath.rbegin()!='/' && *includePath.rbegin()!='\\') {
      includePath += "\\";
    }
#endif

    glswAddPath(includePath.c_str(), ".glsl");
  }

  return hasShaderFiles || hasChildShaderFiles;
}

GLboolean OGLEApplication::setupGLSW()
{
  glswInit();

  // try src directory first, might be more up to date then installation
  boost::filesystem::path srcPath(PROJECT_SOURCE_DIR);
  srcPath /= PROJECT_NAME;
  if(setupGLSWPath(srcPath)) {
    DEBUG_LOG("Loading shader from: " << srcPath);
    return GL_TRUE;
  }

  // if nothing found in src dir, try insatll directory
  boost::filesystem::path installPath(CMAKE_INSTALL_PREFIX);
  installPath /= "share";
  installPath /= PROJECT_NAME;
  installPath /= "shader";
  if(setupGLSWPath(installPath)) {
    DEBUG_LOG("Loading shader from: " << installPath);
    return GL_TRUE;
  }

  return GL_FALSE;
}

void OGLEApplication::initGL()
{
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    cerr << "Error: " << glewGetErrorString(err) << endl;
    exit(1);
  }

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

  if(setupGLSW()==GL_FALSE) {
    ERROR_LOG("Unable to locate shader files.");
    exit(1);
  }

  isGLInitialized_ = GL_TRUE;
}

void OGLEApplication::resizeGL(GLuint width, GLuint height)
{
  glSize_.x = width;
  glSize_.y = height;
  emitEvent(RESIZE_EVENT);
}

void OGLEApplication::drawGL()
{
  boost::posix_time::ptime t(
      boost::posix_time::microsec_clock::local_time());
  GLdouble dt = ((GLdouble)(t-lastDisplayTime_).total_microseconds())/1000.0;
  if(waitForVSync_ && dt < VSYNC_RATE) { return; }
  lastDisplayTime_ = t;

  renderTree_->render(dt);
  swapGL();
  renderTree_->postRender(dt);
}
