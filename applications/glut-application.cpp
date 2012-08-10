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
#include <ogle/animations/animation-manager.h>

#include "glut-application.h"

////////////////////

GlutApplication *GlutApplication::singleton_ = NULL;

void GlutApplication::displayStatic(void)
{
  singleton_->display();
}
void GlutApplication::mouseButtonStatic(int button, int state, int x, int y)
{
  singleton_->mouseButton(button,state,x,y);
}
void GlutApplication::mousePassiveMotionStatic(int x, int y)
{
  singleton_->mousePassiveMotion(x,y);
}
void GlutApplication::mouseMotionStatic(int x, int y)
{
  singleton_->mouseMotion(x,y);
}
void GlutApplication::keyUpStatic(unsigned char key, int x, int y)
{
  singleton_->keyUp(key,x,y);
}
void GlutApplication::keyDownStatic(unsigned char key, int x, int y)
{
  singleton_->keyDown(key,x,y);
}
void GlutApplication::specialKeyUpStatic(int key, int x, int y)
{
  singleton_->specialKeyUp(key,x,y);
}
void GlutApplication::specialKeyDownStatic(int key, int x, int y)
{
  singleton_->specialKeyDown(key,x,y);
}
void GlutApplication::reshapeStatic(int w, int h)
{
  singleton_->reshape(w,h);
}

///////////////////

GlutApplication::GlutApplication(
    int argc, char** argv,
    const string &windowTitle,
    GLuint width,
    GLuint height)
: isButtonDown_(false),
  ctrlPressed_(false),
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
  Logging::addLogger( new CerrLogger(Logging::ERROR) );
  Logging::addLogger( new FileLogger(Logging::FATAL, "ogle-error.log") );
  Logging::addLogger( new CerrLogger(Logging::FATAL) );
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
}

void GlutApplication::exitMainLoop()
{
  applicationRunning_ = false;
}

void GlutApplication::mainLoop()
{
  boost::posix_time::ptime lastt(
      boost::posix_time::microsec_clock::local_time());
  while(applicationRunning_)
  {
    glutMainLoopEvent();

    boost::posix_time::ptime t(
        boost::posix_time::microsec_clock::local_time());
    GLdouble dt = ((GLdouble)(t - lastt).total_microseconds())/1000.0;
    lastt = t;

    handleMainLoopStep(dt);

    if(reshaped_) {
      // do the actual reshaping,
      // glut is just sending to much resize events here for mouse resizing
      handleResize(windowWith_, windowHeight_);
      reshaped_ = false;
    }
  }
}

void GlutApplication::display(void)
{
  boost::posix_time::ptime t(
      boost::posix_time::microsec_clock::local_time());
  GLdouble dt = ((GLdouble)(t-lastDisplayTime_).total_microseconds())/1000.0;
  lastDisplayTime_ = t;
  render(dt);
  glutSwapBuffers();
  glutPostRedisplay();
  postRender(dt);
}

void GlutApplication::mouseButton(int button, int state, int x, int y)
{
  isButtonDown_ = (state == GLUT_DOWN) ? true : false;
  lastMouseX_ = x;
  lastMouseY_ = y;
  if(!isButtonDown_) {
    boost::posix_time::ptime time(
        boost::posix_time::microsec_clock::local_time());
    double milliSeconds = ((double)(time - lastButtonTime_).total_microseconds())/1000.0;
    handleButton(milliSeconds, isButtonDown_, button, x, y);
    lastButtonTime_ = time;
  } else {
    handleButton(0.0, isButtonDown_, button, x, y);
  }
}

void GlutApplication::mousePassiveMotion(int x, int y)
{
}

void GlutApplication::mouseMotion(int x, int y)
{
  boost::posix_time::ptime time(
      boost::posix_time::microsec_clock::local_time());
  GLdouble milliSeconds = ((GLdouble)(time - lastMotionTime_).total_microseconds())/1000.0;
  GLint dx = x - lastMouseX_;
  GLint dy = y - lastMouseY_;
  lastMouseX_ = x;
  lastMouseY_ = y;
  handleMouseMotion(milliSeconds, dx, dy);
  lastMotionTime_ = time;
}

void GlutApplication::keyUp(unsigned char key, int x, int y)
{
  handleKey(true, key);
  keyState_[key] = false;
}

void GlutApplication::keyDown(unsigned char key, int x, int y)
{
  if(key==27) { // escape
    applicationRunning_ = false;
    return;
  }

  handleKey(false, key);
  keyState_[key] = true;
}

void GlutApplication::specialKeyUp(int key, int x, int y)
{
}
void GlutApplication::specialKeyDown(int key, int x, int y)
{
}

void GlutApplication::reshape(int w, int h)
{
  reshaped_ = true;
  windowWith_ = w;
  windowHeight_ = h;
}
