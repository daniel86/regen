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
  };

  /**
   * mouse button event.
   */
  static GLuint BUTTON_EVENT;
  struct ButtonEvent {
    GLboolean pressed;
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

  const Vec2ui& windowSize() const;
  GLuint windowWidth() const;
  GLuint windowHeight() const;

  GLuint mouseX() const;
  GLuint mouseY() const;
  void mouseMove(GLuint x, GLuint y);

  void mouseButton(GLuint button, GLboolean pressed, GLuint x, GLuint y);

  virtual void set_windowTitle(const string&) = 0;

  virtual int mainLoop() = 0;
  virtual void exitMainLoop(int errorCode) = 0;

  virtual void show();

  virtual void initGL();
  virtual void drawGL();
  virtual void swapGL() = 0;
  virtual void resizeGL(GLuint width, GLuint height);

protected:
  OGLERenderTree *renderTree_;

  Vec2ui glSize_;

  boost::posix_time::ptime lastDisplayTime_;
  GLdouble dt_;

  GLint lastMouseX_, lastMouseY_;
  boost::posix_time::ptime lastMotionTime_;
};

#endif // OGLE_APPLICATION_H_

