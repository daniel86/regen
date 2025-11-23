/*
 * glut-application.h
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#ifndef GLUT_APPLICATION_H_
#define GLUT_APPLICATION_H_

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <regen/scene.h>

#include <string>

namespace regen {

#define NUM_KEYS 256

class GLUTApplication : public Application
{
public:
  GLUTApplication(
      const ref_ptr<RootNode> &tree,
      int &argc, char** argv,
      uint32_t width=800, uint32_t height=600);

  void set_windowTitle(const std::string &windowTitle);
  void set_height(uint32_t height);
  void set_width(uint32_t width);
  void set_displayMode(uint32_t displayMode);

  virtual void show();
  virtual int mainLoop();
  virtual void exitMainLoop(int errorCode);

protected:
  static GLUTApplication *singleton_;

  string windowTitle_;
  uint32_t glutHeight_;
  uint32_t glutWidth_;
  uint32_t displayMode_;

  bool applicationRunning_;

  bool reshaped_;

  bool keyState_[NUM_KEYS];
  bool ctrlPressed_;
  bool altPressed_;
  bool shiftPressed_;

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

}

#endif /* GLUT_APPLICATION_H_ */
