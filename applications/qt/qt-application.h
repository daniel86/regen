/*
 * qt-application.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef QT_APPLICATION_H_
#define QT_APPLICATION_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QApplication>
#include <applications/ogle-application.h>
#include <applications/qt/qt-gl-widget.h>
#include <applications/qt/generic-data-window.h>

#include <string>
using namespace std;

namespace ogle {
class QtApplication : public OGLEApplication
{
public:
  QtApplication(
      const ref_ptr<RootNode> &tree,
      int &argc, char** argv,
      GLuint width=800, GLuint height=600,
      QWidget *parent=NULL);
  virtual ~QtApplication();

  QTGLWidget& glWidget()
  { return glWidget_; }

  /**
   * Add generic data to editor, allowing the user to manipulate the data.
   * @param treePath path in tree widget.
   * @param in the data
   * @param minBound per component minimum
   * @param maxBound per component maximum
   * @param precision per component precision
   * @param description brief description
   */
  void addGenericData(
      const string &treePath,
      const ref_ptr<ShaderInput> &in,
      const Vec4f &minBound,
      const Vec4f &maxBound,
      const Vec4i &precision,
      const string &description);

  // OGLEApplication override
  virtual void show();
  virtual int mainLoop();
  virtual void exitMainLoop(int errorCode);
  virtual void set_windowTitle(const string&);
  virtual void swapGL();

protected:
  QApplication app_;
  QTGLWidget glWidget_;
  GenericDataWindow *genericDataWindow_;
};
}

#endif /* QT_OGLE_APPLICATION_H_ */
