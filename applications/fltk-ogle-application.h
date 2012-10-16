/*
 * fltk-application.h
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#ifndef FLTK_APPLICATION_H_
#define FLTK_APPLICATION_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <applications/ogle-application.h>

#include <fltk/GlWindow.h>
#include <fltk/Window.h>
#include <fltk/Widget.h>

#include <string>
using namespace std;

class OGLEFltkApplication : public OGLEApplication
{
public:
  OGLEFltkApplication(
      OGLERenderTree *tree,
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
  string windowTitle_;
  GLuint fltkHeight_;
  GLuint fltkWidth_;

  class GLWindow : public fltk::GlWindow
  {
  public:
    GLWindow(
        OGLEFltkApplication *app,
        GLuint x=0, GLuint y=0,
        GLuint width=800, GLuint height=600,
        const string &title="OGLE - Fltk");
  protected:
    OGLEFltkApplication *app_;
    void layout();
    void draw();
    void flush();
    int handle(int);
  };
  GLWindow fltkWindow_;

  boost::posix_time::ptime lastButtonTime_;

  virtual void initGL();
  virtual void swapGL();
};

#endif /* GLUT_APPLICATION_H_ */
