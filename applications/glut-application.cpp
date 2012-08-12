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
  singleton_->reshaped_ = true;
  singleton_->windowWith_ = w;
  singleton_->windowHeight_ = h;
}

///////////////////

GlutApplication::GlutApplication(
    int argc, char** argv,
    const string &windowTitle,
    GLuint width,
    GLuint height)
: ctrlPressed_(false),
  altPressed_(false),
  shiftPressed_(false),
  reshaped_(false),
  applicationRunning_(true)
{
  singleton_ = this;

  lastMotionTime_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastButtonTime_ = lastMotionTime_;
  lastDisplayTime_ = lastMotionTime_;

  for(GLint i=0; i<NUM_KEYS; ++i) { keyState_[i] = false; }

  glutInit (&argc, argv);
  glutInitWindowSize (width, height);
  glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);
  glutInitDisplayMode(GLUT_RGB);
  //glutInitContextVersion(4,2);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  //glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
  glutCreateWindow(windowTitle.c_str());
  glewInit();

  if (!glewIsSupported("GL_VERSION_3_0"))
  {
    ERROR_LOG("GL_VERSION_3_0 unsupported.");
    exit(-1);
  }
  if (!glewIsSupported("GL_ARB_fragment_program"))
  {
    ERROR_LOG("GL_ARB_fragment_program unsupported.");
    exit(-1);
  }
  if (!glewIsSupported("GL_ARB_vertex_program" ))
  {
    ERROR_LOG("GL_ARB_vertex_program unsupported.");
    exit(-1);
  }
  if (!glewIsSupported("GL_ARB_texture_float"))
  {
    ERROR_LOG("GL_ARB_texture_float unsupported.");
    exit(-1);
  }
  if (!glewIsSupported("GL_ARB_color_buffer_float"))
  {
    ERROR_LOG("GL_ARB_color_buffer_float unsupported.");
    exit(-1);
  }
  if (!glewIsSupported("GL_EXT_framebuffer_object"))
  {
    ERROR_LOG("GL_EXT_framebuffer_object unsupported.");
    exit(-1);
  }

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

  glutMouseFunc(mouseButtonStatic);
  glutMotionFunc(mouseMotionStatic);
  glutPassiveMotionFunc(mousePassiveMotionStatic);
  glutKeyboardFunc(keyDownStatic);
  glutKeyboardUpFunc(keyUpStatic);
  glutSpecialFunc(specialKeyDownStatic);
  glutSpecialUpFunc(specialKeyUpStatic);
  glutDisplayFunc(displayStatic);
  glutReshapeFunc(reshapeStatic);

  glViewport(0, 0, windowWith_, windowHeight_);

  handleGLError("after GlutApplication");
}

GLuint GlutApplication::windowWidth() const
{
  return windowWith_;
}
GLuint GlutApplication::windowHeight() const
{
  return windowHeight_;
}

void GlutApplication::exitMainLoop()
{
  applicationRunning_ = false;
}

void GlutApplication::mainLoop()
{
  while(applicationRunning_)
  {
    glutMainLoopEvent();
    if(reshaped_) {
      glViewport(0, 0, windowWith_, windowHeight_);
      // do the actual reshaping,
      // glut is just sending to much resize events here for mouse resizing
      emit(RESIZE_EVENT);
      reshaped_ = false;
    }
  }
}
