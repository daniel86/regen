/*
 * qt-application.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef QT_APPLICATION_H_
#define QT_APPLICATION_H_

#include <GL/glew.h>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QApplication>
#include <regen/application.h>
#include <applications/qt/qt-gl-widget.h>

#include <string>
using namespace std;

namespace regen {
class QTGLWidget;
class QtApplication : public Application
{
public:
  QtApplication(
      const int &argc, const char** argv,
      const QGLFormat &glFormat,
      GLuint width=800, GLuint height=600,
      QWidget *parent=NULL);

  /**
   * @return topmost parent of GL widget.
   */
  QWidget* toplevelWidget();

  /**
   * @return the rendering widget.
   */
  QTGLWidget* glWidget();
  QWidget* glWidgetContainer();

  void toggleFullscreen();

  void show();

  int mainLoop();
  void exitMainLoop(int errorCode);

protected:
  QApplication *app_;
  QWidget *glContainer_;
  QTGLWidget *glWidget_;
  GLboolean isMainloopRunning_;
  GLint exitCode_;

  friend class QTGLWidget;
};
}

#endif /* QT_APPLICATION_H_ */
