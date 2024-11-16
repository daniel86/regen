#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include "noise-widget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

int main(int argc, char **argv) {
#ifdef Q_WS_X11
#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
	XInitThreads();
#endif
#endif
	QGLFormat glFormat(
			QGL::SingleBuffer
			| QGL::NoAlphaChannel
			| QGL::NoAccumBuffer
			| QGL::NoDepthBuffer
			| QGL::NoStencilBuffer
			| QGL::NoStereoBuffers
			| QGL::NoSampleBuffers);
	glFormat.setSwapInterval(0);
	glFormat.setDirectRendering(true);
	glFormat.setRgba(true);
	glFormat.setOverlay(false);
	glFormat.setVersion(3, 3);
	glFormat.setProfile(QGLFormat::CoreProfile);

	// create and show application window
	ref_ptr<QtApplication> app = ref_ptr<QtApplication>::alloc(argc, (const char **) argv, glFormat);
	app->setupLogging();
	app->toplevelWidget()->setWindowTitle("OpenGL player");

	// create the main widget and connect it to applications key events
	ref_ptr<NoiseWidget> widget = ref_ptr<NoiseWidget>::alloc(app.get());
	widget->show();
	app->show();

	int exitCode = app->mainLoop();

	return exitCode;
}
