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

#include <ogle/utility/event-object.h>

#include <string>
using namespace std;

#define NUM_KEYS 256

class GlutApplication : public EventObject
{
public:
  /**
   * GLUT keyboard event.
   */
  static GLuint KEY_EVENT;
  struct KeyEvent {
    GLdouble dt;
    GLboolean isUp;
    GLint x;
    GLint y;
    unsigned char key;
  };

  /**
   * GLUT mouse button event.
   */
  static GLuint BUTTON_EVENT;
  struct ButtonEvent {
    GLdouble dt;
    GLboolean pressed;
    GLint button;
    GLint x;
    GLint y;
  };

  /**
   * GLUT mouse motion event.
   */
  static GLuint MOUSE_MOTION_EVENT;
  struct MouseMotionEvent {
    GLdouble dt;
    GLint dx;
    GLint dy;
  };

  /**
   * Resize event.
   */
  static GLuint RESIZE_EVENT;

  GlutApplication(int argc, char** argv,
      const string &windowTitle,
      GLuint windowWidth,
      GLuint windowHeight,
      GLuint displayMode=GLUT_RGB|GLUT_SINGLE
      );

  virtual void mainLoop();

  void exitMainLoop();

  const Vec2ui& windowSize() const;
  GLuint windowWidth() const;
  GLuint windowHeight() const;

  virtual void render(GLdouble dt) = 0;
  virtual void postRender(GLdouble dt) = 0;
  virtual void reshape();

protected:
  static GlutApplication *singleton_;

  Vec2ui windowSize_;

  GLboolean applicationRunning_;

  GLboolean reshaped_;

  GLboolean keyState_[NUM_KEYS];
  GLboolean ctrlPressed_;
  GLboolean altPressed_;
  GLboolean shiftPressed_;
  GLint lastMouseX_, lastMouseY_;

  boost::posix_time::ptime lastMotionTime_;
  boost::posix_time::ptime lastButtonTime_;
  boost::posix_time::ptime lastDisplayTime_;

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
