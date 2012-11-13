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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Pack.H>

#include <string>
using namespace std;

class OGLEFltkApplication : public OGLEApplication
{
public:
  OGLEFltkApplication(
      OGLERenderTree *tree,
      int &argc, char** argv,
      GLuint width=800, GLuint height=600);

  virtual void createWidgets(Fl_Pack *parent);
  void createShaderInputWidget();

  void addShaderInput(ref_ptr<ShaderInput1f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(ref_ptr<ShaderInput2f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(ref_ptr<ShaderInput3f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(ref_ptr<ShaderInput4f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(ref_ptr<ShaderInput1i> &in,
      GLint min, GLint max, GLint step);
  void addShaderInput(ref_ptr<ShaderInput2i> &in,
      const Vec2i& min, const Vec2i& max, const Vec2i& step);
  void addShaderInput(ref_ptr<ShaderInput3i> &in,
      const Vec3i& min, const Vec3i& max, const Vec3i& step);
  void addShaderInput(ref_ptr<ShaderInput4i> &in,
      const Vec4i& min, const Vec4i& max, const Vec4i& step);

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

  Fl_Window mainWindow_;
  Fl_Pack *mainWindowPackH_;
  Fl_Pack *mainWindowPackV_;

  GLboolean isApplicationRunning_;

  class GLWindow : public Fl_Gl_Window
  {
  public:
    GLWindow(
        OGLEFltkApplication *app,
        GLint x=0, GLint y=0,
        GLint width=800, GLint height=600);
  protected:
    OGLEFltkApplication *app_;
    void resize(int x, int y, int w, int h);
    void draw();
    void flush();
    int handle(int);
  };
  GLWindow *fltkWindow_;

  boost::posix_time::ptime lastButtonTime_;

  virtual void initGL();
  virtual void swapGL();
};

#endif /* GLUT_APPLICATION_H_ */
