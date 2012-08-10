/*
 * glut-application.h
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#ifndef GLUT_APPLICATION_H_
#define GLUT_APPLICATION_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>

#include <boost/thread/thread.hpp>

#include <string>
using namespace std;

#define NUM_KEYS 256

// TODO: extend event object and offer some events

class GlutApplication
{
public:
  GlutApplication(int argc, char** argv,
      const string &windowTitle,
      GLuint windowWidth,
      GLuint windowHeight
      );

  void mainLoop();

  void exitMainLoop();

  virtual void render(GLdouble dt) = 0;
  virtual void postRender(GLdouble dt) = 0;

  virtual void handleKey(
      GLboolean isUp,
      unsigned char key)
  {}
  virtual void handleMouseMotion(
      GLdouble dt,
      GLint x,
      GLint y)
  {}
  virtual void handleButton(
      GLdouble dt,
      GLboolean isDown,
      GLint button,
      GLint x,
      GLint y)
  {}
  virtual void handleResize(
      GLint w,
      GLint h)
  {}
  virtual void handleMainLoopStep(
      GLfloat dt)
  {}

protected:
  static GlutApplication *singleton_;

  GLint windowWith_, windowHeight_;

  GLboolean applicationRunning_;

  GLboolean reshaped_;

  GLboolean keyState_[NUM_KEYS];
  GLboolean isButtonDown_;
  GLboolean ctrlPressed_;
  GLboolean altPressed_;
  GLboolean shiftPressed_;
  GLint lastMouseX_, lastMouseY_;

  boost::posix_time::ptime lastMotionTime_;
  boost::posix_time::ptime lastButtonTime_;
  boost::posix_time::ptime lastDisplayTime_;

  void display(void);
  void mouseButton(int button, int state, int x, int y);
  void mousePassiveMotion(int x, int y);
  void mouseMotion(int x, int y);
  void keyUp(unsigned char key, int x, int y);
  void keyDown(unsigned char key, int x, int y);
  void specialKeyUp(int key, int x, int y);
  void specialKeyDown(int key, int x, int y);
  void reshape(int w, int h);

  // glut event handler, can only be static :/
  static void displayStatic(void);
  static void mouseButtonStatic(int button, int state, int x, int y);
  static void mousePassiveMotionStatic(int x, int y);
  static void mouseMotionStatic(int x, int y);
  static void keyUpStatic(unsigned char key, int x, int y);
  static void keyDownStatic(unsigned char key, int x, int y);
  static void specialKeyUpStatic(int key, int x, int y);
  static void specialKeyDownStatic(int key, int x, int y);
  static void reshapeStatic(int w, int h);
};

#endif /* GLUT_APPLICATION_H_ */
