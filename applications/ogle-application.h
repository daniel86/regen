/*
 * ogle-application.h
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#ifndef OGLE_APPLICATION_H_
#define OGLE_APPLICATION_H_

#include <ogle/algebra/vector.h>
#include <ogle/utility/logging.h>
#include <ogle/utility/event-object.h>

#include <applications/ogle-render-tree.h>
#include <applications/application-config.h>

#define OGLE_MOUSE_BUTTON_LEFT    1
#define OGLE_MOUSE_BUTTON_RIGHT   2
#define OGLE_MOUSE_BUTTON_MIDDLE  3
#define OGLE_MOUSE_WHEEL_UP       4
#define OGLE_MOUSE_WHEEL_DOWN     5

class OGLEApplication : public EventObject
{
public:
  /**
   * keyboard event.
   */
  static GLuint KEY_EVENT;
  struct KeyEvent {
    GLdouble dt;
    GLboolean isUp;
    GLint x;
    GLint y;
    unsigned char key;
    int keyValue;
  };

  /**
   * mouse button event.
   */
  static GLuint BUTTON_EVENT;
  struct ButtonEvent {
    GLboolean pressed;
    GLboolean isDoubleClick;
    GLint button;
    GLint x;
    GLint y;
  };

  /**
   * mouse motion event.
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

  OGLEApplication(
      OGLERenderTree *tree,
      int &argc, char** argv,
      GLuint width, GLuint height);

  void setWaitForVSync(GLboolean v);

  const Vec2ui& windowSize() const;
  GLuint windowWidth() const;
  GLuint windowHeight() const;

  GLuint mouseX() const;
  GLuint mouseY() const;
  virtual void mouseMove(GLint x, GLint y);

  virtual void mouseButton(GLuint button, GLboolean pressed, GLuint x, GLuint y, GLboolean isDoubleClick=GL_FALSE);

  virtual void keyUp(int key, GLuint x, GLuint y);
  virtual void keyDown(int key, GLuint x, GLuint y);

  virtual void set_windowTitle(const string&) = 0;

  virtual int mainLoop() = 0;
  virtual void exitMainLoop(int errorCode) = 0;

  virtual void initGL();
  virtual void initTree();
  virtual void drawGL();
  virtual void swapGL() = 0;
  virtual void resizeGL(GLuint width, GLuint height);

protected:
  OGLERenderTree *renderTree_;

  Vec2ui glSize_;

  boost::posix_time::ptime lastDisplayTime_;
  GLdouble dt_;

  GLboolean waitForVSync_;

  GLint lastMouseX_, lastMouseY_;
  boost::posix_time::ptime lastMotionTime_;
};

#endif // OGLE_APPLICATION_H_

