/*
 * glut-application.cpp
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>

#include <ogle/font/font-manager.h>
#include <ogle/utility/logging.h>
#include <ogle/utility/gl-error.h>
#include <ogle/animations/animation-manager.h>

#include "glut-application.h"

////////////////////

GLuint GlutApplication::BUTTON_EVENT =
    EventObject::registerEvent("glutAppButton");
GLuint GlutApplication::KEY_EVENT =
    EventObject::registerEvent("glutAppKey");
GLuint GlutApplication::MOUSE_MOTION_EVENT =
    EventObject::registerEvent("glutAppMouseMtion");
GLuint GlutApplication::RESIZE_EVENT =
    EventObject::registerEvent("glutAppResize");

GlutApplication *GlutApplication::singleton_ = NULL;

void GlutApplication::displayStatic(void)
{
  boost::posix_time::ptime t(
      boost::posix_time::microsec_clock::local_time());
  GLdouble dt = ((GLdouble)(t-singleton_->lastDisplayTime_).total_microseconds())/1000.0;
  singleton_->lastDisplayTime_ = t;
  singleton_->render(dt);
  glutSwapBuffers();
  glutPostRedisplay();
  singleton_->postRender(dt);
}
void GlutApplication::mouseButtonStatic(int button, int state, int x, int y)
{
  singleton_->lastMouseX_ = x;
  singleton_->lastMouseY_ = y;

  ButtonEvent event;
  event.button = button;
  event.x = x;
  event.y = y;

  if(state != GLUT_DOWN) {
    boost::posix_time::ptime time(
        boost::posix_time::microsec_clock::local_time());
    double milliSeconds = ((double)(time - singleton_->lastButtonTime_).total_microseconds())/1000.0;

    event.dt = milliSeconds;
    event.pressed = false;

    singleton_->lastButtonTime_ = time;
  } else {
    event.dt = 0.0;
    event.pressed = true;
  }

  singleton_->emit(BUTTON_EVENT, &event);
}
void GlutApplication::mousePassiveMotionStatic(int x, int y)
{
  mouseMotionStatic(x,y);
}
void GlutApplication::mouseMotionStatic(int x, int y)
{
  boost::posix_time::ptime time(
      boost::posix_time::microsec_clock::local_time());
  GLdouble milliSeconds = ((GLdouble)(time - singleton_->lastMotionTime_).total_microseconds())/1000.0;
  GLint dx = x - singleton_->lastMouseX_;
  GLint dy = y - singleton_->lastMouseY_;
  singleton_->lastMouseX_ = x;
  singleton_->lastMouseY_ = y;

  MouseMotionEvent event;
  event.dt = milliSeconds;
  event.dx = dx;
  event.dy = dy;
  singleton_->emit(MOUSE_MOTION_EVENT, &event);

  singleton_->lastMotionTime_ = time;
}
void GlutApplication::keyUpStatic(unsigned char key, int x, int y)
{
  KeyEvent event;
  event.isUp = true;
  event.key = key;
  event.x = x;
  event.y = y;
  singleton_->emit(KEY_EVENT, &event);

  singleton_->keyState_[key] = false;
}
void GlutApplication::keyDownStatic(unsigned char key, int x, int y)
{
  if(key==27) { // escape
    singleton_->applicationRunning_ = false;
    return;
  }

  KeyEvent event;
  event.isUp = false;
  event.key = key;
  event.x = x;
  event.y = y;
  singleton_->emit(KEY_EVENT, &event);

  singleton_->keyState_[key] = true;
}
void GlutApplication::specialKeyUpStatic(int key, int x, int y)
{
}
void GlutApplication::specialKeyDownStatic(int key, int x, int y)
{
}
void GlutApplication::reshapeStatic(int w, int h)
{
  if(w!=singleton_->windowSize_.x || h!=singleton_->windowSize_.y)
  {
    singleton_->reshaped_ = true;
    singleton_->windowSize_.x = w;
    singleton_->windowSize_.y = h;
  }
}

///////////////////

GlutApplication::GlutApplication(
    int argc, char** argv,
    const string &windowTitle,
    GLuint width,
    GLuint height,
    GLuint displayMode)
: ctrlPressed_(false),
  altPressed_(false),
  shiftPressed_(false),
  reshaped_(false),
  applicationRunning_(true),
  windowSize_(width,height)
{
  singleton_ = this;

  lastMotionTime_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastButtonTime_ = lastMotionTime_;
  lastDisplayTime_ = lastMotionTime_;

  for(GLint i=0; i<NUM_KEYS; ++i) { keyState_[i] = false; }

  glutInit(&argc, argv);

  glutInitContextVersion(3, 0);
  // for example FBO is core since 4.3, that's why
  // we cannot set GLUT_FORWARD_COMPATIBLE here :/
  // glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
  glutInitContextProfile(GLUT_CORE_PROFILE);

  glutInitWindowSize (width, height);
  glutInitDisplayMode(displayMode);

  glutCreateWindow(windowTitle.c_str());

  glutSetOption(
      GLUT_ACTION_ON_WINDOW_CLOSE,
      GLUT_ACTION_EXIT
      );
  glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);

  glewInit();

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

  DEBUG_LOG("Vendor: " << glGetString(GL_VENDOR));
  DEBUG_LOG("Renderer: " << glGetString(GL_RENDERER));
  DEBUG_LOG("Version: " << glGetString(GL_VERSION));
  DEBUG_LOG("GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION));

  if (!glewIsSupported("GL_EXT_framebuffer_object"))
  {
    ERROR_LOG("GL_EXT_framebuffer_object unsupported.");
    exit(-1);
  }

  glutMouseFunc(mouseButtonStatic);
  glutMotionFunc(mouseMotionStatic);
  glutPassiveMotionFunc(mousePassiveMotionStatic);
  glutKeyboardFunc(keyDownStatic);
  glutKeyboardUpFunc(keyUpStatic);
  glutSpecialFunc(specialKeyDownStatic);
  glutSpecialUpFunc(specialKeyUpStatic);
  glutDisplayFunc(displayStatic);
  glutReshapeFunc(reshapeStatic);

  glViewport(0, 0, windowSize_.x, windowSize_.y);

  // set some default states
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  // specify whether front- or back-facing facets can be culled
  glCullFace(GL_BACK);
  // define front- and back-facing polygons
  glFrontFace(GL_CCW);
}

const Vec2ui& GlutApplication::windowSize() const
{
  return windowSize_;
}
GLuint GlutApplication::windowWidth() const
{
  return windowSize_.x;
}
GLuint GlutApplication::windowHeight() const
{
  return windowSize_.y;
}

GLuint GlutApplication::mouseX() const
{
  return lastMouseX_;
}
GLuint GlutApplication::mouseY() const
{
  return lastMouseY_;
}

void GlutApplication::exitMainLoop()
{
  applicationRunning_ = false;
}

void GlutApplication::reshape()
{
  glViewport(0, 0, windowSize_.x, windowSize_.y);
  // do the actual reshaping
  emit(RESIZE_EVENT);
  reshaped_ = false;
}

void GlutApplication::mainLoop()
{
  AnimationManager::get().resume();

  while(applicationRunning_)
  {
    glutMainLoopEvent();
    if(reshaped_)
    {
      reshape();
    }
  }
}
