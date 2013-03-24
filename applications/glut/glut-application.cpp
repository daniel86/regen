/*
 * glut-application.cpp
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>

#include <regen/utility/font-manager.h>
#include <regen/utility/logging.h>
#include <regen/utility/gl-util.h>
#include <regen/animations/animation-manager.h>
#include <regen/external/glsw/glsw.h>

#include "glut-application.h"
using namespace ogle;

////////////////////

GLUTApplication *GLUTApplication::singleton_ = NULL;

void GLUTApplication::displayStatic(void)
{
  singleton_->drawGL();
}
void GLUTApplication::reshapeStatic(int w, int h)
{
  if(w!=(int)singleton_->glSize_.x || h!=(int)singleton_->glSize_.y)
  {
    singleton_->reshaped_ = true;
    singleton_->glutHeight_ = h;
    singleton_->glutWidth_ = w;
  }
}
void GLUTApplication::mousePassiveMotionStatic(int x, int y)
{
  mouseMotionStatic(x,y);
}
void GLUTApplication::mouseMotionStatic(int x, int y)
{
  singleton_->mouseMove(x,singleton_->glutHeight_-y);
}
void GLUTApplication::mouseButtonStatic(int button, int state, int x, int y)
{
  singleton_->mouseButton(button,state==GLUT_DOWN,x,singleton_->glutHeight_-y);
}
void GLUTApplication::keyUpStatic(unsigned char key, int x, int y)
{
  singleton_->keyDown(key,x,y);
  singleton_->keyState_[key] = false;

}
void GLUTApplication::keyDownStatic(unsigned char key, int x, int y)
{
  if(key==27) { // escape
    singleton_->applicationRunning_ = false;
    return;
  }
  singleton_->keyUp(key,x,y);
  singleton_->keyState_[key] = true;
}
void GLUTApplication::specialKeyUpStatic(int key, int x, int y)
{
}
void GLUTApplication::specialKeyDownStatic(int key, int x, int y)
{
}

///////////////////

GLUTApplication::GLUTApplication(
    int &argc, char** argv,
    GLuint width, GLuint height)
: OGLEApplication(argc,argv),
  windowTitle_("OpenGL Engine"),
  glutHeight_(width),
  glutWidth_(height),
  displayMode_(GLUT_RGB),
  applicationRunning_(true),
  reshaped_(false),
  ctrlPressed_(false),
  altPressed_(false),
  shiftPressed_(false)
{
  resizeGL(Vec2i(width,height));
  singleton_ = this;

  lastButtonTime_ = lastMotionTime_;

  for(GLint i=0; i<NUM_KEYS; ++i) { keyState_[i] = false; }

  glutInit(&argc, argv);
}

void GLUTApplication::set_windowTitle(const string &windowTitle)
{
  windowTitle_ = windowTitle;
}
void GLUTApplication::set_height(GLuint height)
{
  glutHeight_ = height;
}
void GLUTApplication::set_width(GLuint width)
{
  glutWidth_ = width;
}
void GLUTApplication::set_displayMode(GLuint displayMode)
{
  displayMode_ = displayMode;
}

void GLUTApplication::initGL()
{
  glutInitContextVersion(3, 0);
  // for example FBO is core since 4.3, that's why
  // we cannot set GLUT_FORWARD_COMPATIBLE here :/
  // glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
  glutInitContextProfile(GLUT_CORE_PROFILE);

  glutInitWindowSize(glutWidth_, glutHeight_);
  glutInitDisplayMode(displayMode_);

  glutCreateWindow(windowTitle_.c_str());

  glutSetOption(
      GLUT_ACTION_ON_WINDOW_CLOSE,
      GLUT_ACTION_EXIT
      );
  glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);

  glutMouseFunc(mouseButtonStatic);
  glutMotionFunc(mouseMotionStatic);
  glutPassiveMotionFunc(mousePassiveMotionStatic);
  glutKeyboardFunc(keyDownStatic);
  glutKeyboardUpFunc(keyUpStatic);
  glutSpecialFunc(specialKeyDownStatic);
  glutSpecialUpFunc(specialKeyUpStatic);
  glutDisplayFunc(displayStatic);
  glutReshapeFunc(reshapeStatic);

  OGLEApplication::initGL();
}

void GLUTApplication::exitMainLoop(int errorCode)
{
  applicationRunning_ = false;
}

void GLUTApplication::show()
{
  initGL();
}

void GLUTApplication::swapGL()
{
  glutSwapBuffers();
  glutPostRedisplay();
}

int GLUTApplication::mainLoop()
{
  AnimationManager::get().resume();

  while(applicationRunning_)
  {
    glutMainLoopEvent();
    if(reshaped_)
    {
      resizeGL(glutWidth_, glutHeight_);
      reshaped_ = false;
    }
  }
  return 0;
}
