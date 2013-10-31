
#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include "scene-display-widget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif
#include <QtGui/QFileDialog>
#include <QtCore/QString>

#include <regen/utility/filesystem.h>
#include <regen/scenes/scene-xml.h>

#define CONFIG_FILE_NAME ".regen-scene-display.cfg"

using namespace regen;

int main(int argc, char** argv)
{
#ifdef Q_WS_X11
#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
  XInitThreads();
#endif
#endif
  QGLFormat glFormat(
    QGL::SingleBuffer
   |QGL::NoAlphaChannel
   |QGL::NoAccumBuffer
   |QGL::NoDepthBuffer
   |QGL::NoStencilBuffer
   |QGL::NoStereoBuffers
   |QGL::NoSampleBuffers);
  glFormat.setSwapInterval(0);
  glFormat.setDirectRendering(true);
  glFormat.setRgba(true);
  glFormat.setOverlay(false);
  glFormat.setVersion(3,3);
  glFormat.setProfile(QGLFormat::CoreProfile);

  // create and show application window
  ref_ptr<QtApplication> app = ref_ptr<QtApplication>::alloc(argc,(const char**)argv,glFormat);
  app->setupLogging();
  app->toplevelWidget()->setWindowTitle("Scene Viewer");
  app->show();

  SceneDisplayWidget *widget = new SceneDisplayWidget(app.get());
  widget->show();
  widget->init();

  return app->mainLoop();
}