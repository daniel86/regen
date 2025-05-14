#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include "mesh-viewer-widget.h"
#include "applications/qt/scene-widget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

int main(int argc, char **argv) {
#ifdef Q_WS_X11
#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
	XInitThreads();
#endif
#endif
	// create and show application window
	ref_ptr<QtApplication> app = ref_ptr<QtApplication>::alloc(
			argc, (const char **) argv,
			SceneWidget::defaultFormat());
	app->setupLogging();
	app->setVSyncEnabled(true);
	app->toplevelWidget()->setWindowTitle("REGEN Mesh Viewer");

	// create the main widget and connect it to applications key events
	ref_ptr<MeshViewerWidget> widget = ref_ptr<MeshViewerWidget>::alloc(app.get());
	widget->show();
	app->show();

	int exitCode = app->mainLoop();

	return exitCode;
}
