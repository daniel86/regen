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

#include <applications/ogle-application.h>

#include <string>
using namespace std;

#define NUM_KEYS 256

class OGLEGlutApplication : public OGLEApplication
{
public:
  OGLEGlutApplication(
      const ref_ptr<RenderTree> &tree,
      int &argc, char** argv,
      GLuint width=800, GLuint height=600);

  void set_windowTitle(const string &windowTitle);
  void set_height(GLuint height);
  void set_width(GLuint width);
  void set_displayMode(GLuint displayMode);

  virtual void show();
  virtual int mainLoop();
  virtual void exitMainLoop(int errorCode);

protected:
  static OGLEGlutApplication *singleton_;

  string windowTitle_;
  GLuint glutHeight_;
  GLuint glutWidth_;
  GLuint displayMode_;

  GLboolean applicationRunning_;

  GLboolean reshaped_;

  GLboolean keyState_[NUM_KEYS];
  GLboolean ctrlPressed_;
  GLboolean altPressed_;
  GLboolean shiftPressed_;

  boost::posix_time::ptime lastButtonTime_;

  virtual void initGL();
  virtual void swapGL();

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
