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
#include <ogle/external/glsw/glsw.h>

#include "glut-ogle-application.h"

////////////////////

OGLEGlutApplication *OGLEGlutApplication::singleton_ = NULL;

void OGLEGlutApplication::displayStatic(void)
{
  singleton_->drawGL();
}
void OGLEGlutApplication::reshapeStatic(int w, int h)
{
  if(w!=singleton_->glSize_.x || h!=singleton_->glSize_.y)
  {
    singleton_->reshaped_ = true;
    singleton_->glutHeight_ = h;
    singleton_->glutWidth_ = w;
  }
}
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
void OGLEGlutApplication::keyUpStatic(unsigned char key, int x, int y)
{
  singleton_->keyDown(key,x,y);
  singleton_->keyState_[key] = false;

}
void OGLEGlutApplication::keyDownStatic(unsigned char key, int x, int y)
{
  if(key==27) { // escape
    singleton_->applicationRunning_ = false;
    return;
  }
  singleton_->keyUp(key,x,y);
  singleton_->keyState_[key] = true;
}
void OGLEGlutApplication::specialKeyUpStatic(int key, int x, int y)
{
}
void OGLEGlutApplication::specialKeyDownStatic(int key, int x, int y)
{
}

///////////////////

OGLEGlutApplication::OGLEGlutApplication(
    OGLERenderTree *tree,
    int &argc, char** argv,
    GLuint width, GLuint height)
: OGLEApplication(tree,argc,argv,width,height),
  ctrlPressed_(false),
  altPressed_(false),
  shiftPressed_(false),
  reshaped_(false),
  applicationRunning_(true),
  windowTitle_("OpenGL Engine"),
  displayMode_(GLUT_RGB),
  glutHeight_(width),
  glutWidth_(height)
{
  singleton_ = this;

  lastButtonTime_ = lastMotionTime_;

  for(GLint i=0; i<NUM_KEYS; ++i) { keyState_[i] = false; }

  glutInit(&argc, argv);
}

void OGLEGlutApplication::set_windowTitle(const string &windowTitle)
{
  windowTitle_ = windowTitle;
}
void OGLEGlutApplication::set_height(GLuint height)
{
  glutHeight_ = height;
}
void OGLEGlutApplication::set_width(GLuint width)
{
  glutWidth_ = width;
}
void OGLEGlutApplication::set_displayMode(GLuint displayMode)
{
  displayMode_ = displayMode;
}

void OGLEGlutApplication::initGL()
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

void OGLEGlutApplication::exitMainLoop(int errorCode)
{
  applicationRunning_ = false;
}

void OGLEGlutApplication::show()
{
  initGL();
  initTree();
}

void OGLEGlutApplication::swapGL()
{
  glutSwapBuffers();
  glutPostRedisplay();
}

int OGLEGlutApplication::mainLoop()
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
